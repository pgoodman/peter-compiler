/*
 * p-parser.c
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-parser.h>

#define PARSER_DEBUG(x) x
#define D(x) PARSER_DEBUG(x)

#define IR_FAILED ((void *) 10)
#define IR_INITIAL ((void *) 11)

/**
 * Initialize the parser data structure.
 */
static void P_init(PParser *parser,
                   PGrammar *grammar,
                   PToken tokens[],
                   int num_tokens) {

    PT_Set *tree_set;
    PT_Terminal *first_terminal;
    int i;

    assert_not_null(grammar);

    tree_set = PTS_alloc();
    first_terminal = PT_alloc_terminals(tree_set, tokens, num_tokens);

    /*
    if(token_result.num_tokens == 0) {
        PTS_free(tree_set);
        return NULL;
    }
    */

    parser->trees = tree_set;
    parser->num_tokens = num_tokens;
    parser->farthest_id_reached = 0;
    parser->call.frame = -1;
    parser->must_backtrack = 0;

    G_lock(grammar);

    /* allocate the cache table */
    parser->intermediate_results = mem_calloc(
        (parser->num_tokens + 1) * grammar->num_productions,
        sizeof(P_IntermediateResult *)
    );

    /* clear out the frame stack */
    for(i = 0; i < P_MAX_RECURSION_DEPTH; ++i) {
        parser->call.stack[i] = NULL;
    }
}

/* -------------------------------------------------------------------------- */

/**
 * Free all of the parser's stack frames.
 */
static void F_free_all(PParser *parser) {

    unsigned int i;

    register P_Frame **stack = (P_Frame **) parser->call.stack;

    for(i = 0; i < P_MAX_RECURSION_DEPTH && is_not_null(stack[i]); ++i) {
        mem_free(stack[i]);
        stack[i] = NULL;
    }

    parser->call.frame = -1;
}

/**
 * Update the tree of one of the stack frames.
 */
static void F_record_tree(P_Frame *frame, PParseTree *parse_tree) {
    PT_add_branch(
        frame->parse_tree,
        parse_tree,
        G_production_rule_get_symbol(
            frame->production.rule,
            frame->production.phrase,
            frame->production.symbol
        )
    );
}

/**
 * Update one of the stack frames as a successful application of a non-terminal
 * to the token stream.
 */
static void F_update_success(PParser *parser,
                             unsigned int which_frame,
                             P_IntermediateResult *result) {

    P_Frame *frame = parser->call.stack[which_frame],
            *caller;

    result->intermediate_tree = frame->parse_tree;

    /* record a successful application of this frame. */
    if(which_frame > 0) {
        caller = parser->call.stack[which_frame - 1];

        result->intermediate_tree = frame->parse_tree;
        F_record_tree(caller, frame->parse_tree);

        ++(caller->production.symbol);
    }
}

/**
 * Push a new stack frame onto the parser's frame stack.
 */
static P_Frame *F_push(PParser *parser) {

    unsigned int i;

    ++(parser->call.frame);

    /* TODO better error here */
    if(parser->call.frame >= P_MAX_RECURSION_DEPTH) {
        printf("Parse Error: maximum recursion depth exceeded.\n");
        exit(1);
    }

    i = parser->call.frame;

    if(is_null(parser->call.stack[i])) {
        parser->call.stack[i] = mem_calloc(1, sizeof(P_Frame));
        if(is_null(parser->call.stack[i])) {
            mem_error("Unable to allocate new parser frame.");
        }
    }

    return parser->call.stack[i];
}

/**
 * Initialize a parser stack frame.
 */
static P_Frame *F_init(P_Frame *frame,
                       G_ProductionRule *rule,
                       PT_Terminal *backtrack_point) {

    frame->backtrack_point = backtrack_point;
    frame->farthest_id_reached = backtrack_point->id;

    frame->left_recursion.is_direct = 0;
    frame->left_recursion.is_used = 0;
    frame->left_recursion.symbol = NULL;

    frame->production.phrase = 0;
    frame->production.symbol = 0;
    frame->production.rule = rule;

    /* TODO start bigger.. ? */
    frame->parse_tree = (PParseTree *) PT_alloc_non_terminal(
        rule->production,
        2
    );

    return frame;
}

/* -------------------------------------------------------------------------- */

/**
 * Free the intermediate results table.
 */
static void IR_free_all(PParser *parser, PGrammar *grammar) {

    register uint32_t i,
                      j;

    uint32_t i_max = grammar->num_productions,
             j_max = parser->num_tokens,
             k;

    register P_IntermediateResult **results = parser->intermediate_results;

    for(i = 0; i < i_max; ++i) {
        k = i * j_max;
        for(j = 0; j < j_max; ++j) {
            if(is_not_null(results[k + j])) {
                mem_free(results[k + j]);
            }
        }
    }

    mem_free(results);

    parser->intermediate_results = NULL;
    parser->num_tokens = 0;
}

/**
 * Get an intermediate result.
 */
static P_IntermediateResult *IR_get(PParser *parser,
                                    G_NonTerminal production,
                                    uint32_t id) {
    assert(id <= parser->num_tokens);

    D( printf("intermediate result for production %d, token %d \n", (int) production, (int) id); )

    return ((parser->intermediate_results)
        [((uint32_t) production * parser->num_tokens) + (uint32_t) id]
    );
}

/**
 * Create the initial intermediate result.
 */
static void IR_create(PParser *parser, G_NonTerminal production, uint32_t id) {

    P_IntermediateResult *result;
    uint32_t i = ((uint32_t) production * parser->num_tokens) + (uint32_t) id;

    assert(id <= parser->num_tokens);
    assert_null(parser->intermediate_results[i]);

    result = mem_alloc(sizeof(P_IntermediateResult));
    if(is_null(result)) {
        mem_error("Unable to allocate new itermediate result.");
    }

    result->end_token = NULL;
    result->intermediate_tree = IR_INITIAL;
    result->is_left_recursive = 0;

    parser->intermediate_results[i] = result;
}

/* -------------------------------------------------------------------------- */

/**
 * Check whether an intermediate result is left-recursive and has a left-
 * recursive seed.
 */
static int LR_intermediate_result_has_seed(P_IntermediateResult *result) {
    assert_not_null(result);

    return result->intermediate_tree != IR_INITIAL
        && result->intermediate_tree != IR_FAILED
        && result->is_left_recursive;
}

/**
 * Grow a left-recursive seed.
 */
static PT_Terminal *LR_grow(PParser *parser,
                            P_IntermediateResult *result,
                            PT_Terminal *token) {

    P_Frame *frame = parser->call.stack[(unsigned int) parser->call.frame],
            *caller;
    int i;

    /* this is the original starter of the left recursion, grow it from here. */
    if(frame->left_recursion.is_used) {

        /* extend the frame tree */
        if(LR_intermediate_result_has_seed(result)) {
            PT_add_branch(
                frame->parse_tree,
                result->intermediate_tree,
                frame->left_recursion.symbol
            );
        }

        /* restart the application of this frame if progress was made and
         * update the cached tree with what progress we made. */
        if(frame->farthest_id_reached < token->id) {

            result->intermediate_tree = frame->parse_tree;

            frame->farthest_id_reached = token->id;
            frame->production.symbol = 0;
            frame->production.phrase = 0;

            return frame->backtrack_point;

        /* no progress was made, time to backtrack. */
        } else {

            /* go down the frame stack and remove consecutive uses of left
             * recursion. */
            for(i = parser->call.frame; i >= 0; --i) {
                frame = parser->call.stack[i];

                if(!frame->left_recursion.is_used) {
                    break;
                }

                /* record a successful application of this frame. */
                if(i > 0) {
                    caller = parser->call.stack[i - 1];

                    F_update_success(parser, i, result);

                    /* get the next cached result */
                    result = IR_get(
                        parser,
                        caller->production.rule->production,
                        caller->backtrack_point->id
                    );
                }
            }

            parser->call.frame = i;
            return token;
        }


    /* this is not the original starter of left recursion, don't grow it until
     * we get to the original one. */
    } else {
        F_update_success(parser, parser->call.frame, result);
        return token;
    }
}

/**
 * Search the parser stack for the production that originated the left
 * recursive rule on the top of the parser stack.
 */
static void LR_mark_origin(PParser *parser) {

    int i = parser->call.frame;
    P_Frame *frame = parser->call.stack[i],
            *origin = parser->call.stack[--i];

    for(; i >= 0; --i) {
        if(origin->production.rule->production
        == frame->production.rule->production) {

            origin->left_recursion.is_used = 1;
            origin->left_recursion.is_direct = ((i+1) == parser->call.frame);

            origin->left_recursion.symbol = G_production_rule_get_symbol(
                origin->production.rule,
                origin->production.phrase,
                origin->production.symbol
            );

            break;
        }
    }
}

/* -------------------------------------------------------------------------- */

/**
 * Parse the tokens from a token generator. This parser operates on a simplified
 * TDPL (Top-Down Parsing Language). It supports *local* backtracking. As such,
 * this parser cannot be used to parse ambiguous grammar because expression rules
 * in productions are ordered and once one succeeds for a particular input the
 * other possibilities will never be attempted for that input.
 *
 * The parser starts by accumulating all of the tokens from the source text
 * into a large linked list.
 *
 * The parser then gets things started by pushing on a new stack frame
 * representing the application of the parser's starting production to the
 * the entire list of tokens, starting from the first token.
 *
 * At all times we maintain a pointer to where we are in the list of tokens.
 * This position can shift dramatically in the event that we don't match a
 * non/terminal and hence have to backtrack, or if we've previously matched
 * a production and hence have to jump ahead because we don't need to repeat
 * work that we've already done.
 *
 * All results from productions are cached. That is, if a production fails then
 * we cache that failure in the thunk table as a special pointer. If a production
 * succeeds then we cache its parse tree and where in the token list it ended.
 *
 * This parser produces a reduced concrete syntax tree (parse tree). It is
 * similar to an abstract syntax tree in that it is much smaller than a typical
 * parse tree and, for the most part, only contains information that is relevant
 * to semantic analysis. The reduced parse tree is created from the parse tree
 * by shaving off unnecessary nodes and by excluding non-useful tokens from the
 * tree itself.
 *
 * TODO: i) implement useful parse error reporting and recovery
 *       ii) allow for left-recursion, as described in the following article:
 *           http://portal.acm.org/citation.cfm?id=1328408.1328424
 */
PParseTree *parse_tokens(PGrammar *grammar, PToken tokens[], int num_tokens) {

    PParser parser;
    P_Frame *frame = NULL;
    P_IntermediateResult *intermediate_result = NULL;
    G_Symbol *symbol = NULL;
    PParseTree *temp_parse_tree = NULL;
    PTreeGenerator *tree_generator = NULL;
    PT_Terminal *token = NULL;

    /* useful counter, be it for hinting to GCC that a block shouldn't be
     * optimized out, or for actually counting something. */
    unsigned int j = 0;

    D( printf("initializing parser.\n"); )

    P_init(&parser, grammar, tokens, num_tokens);

    /* get all of the tokens as terminal trees */
    token = PT_alloc_terminals(
        parser.trees,
        tokens,
        num_tokens
    );

    /* get the starting production and push our first stack frame on. This
     * involves registering the start of the token list as the farthest back
     * we can backtrack.
     */
    D( printf("pushing production onto stack.\n"); )

    /* push on our first stack frame */
    frame = F_init(
        F_push(&parser),
        grammar->production_rules + grammar->start_production_rule,
        token
    );
    PTS_add(parser.trees, frame->parse_tree);
    IR_create(&parser, frame->production.rule->production, 0);

begin_parsing:

    D( printf("beginning main parse.\n"); )

    while(0 <= parser.call.frame) {

        frame = parser.call.stack[j = parser.call.frame];

        D( printf("\nframe is %p at %d \n", (void *) frame, (int) parser.call.frame); )

        /* get the cached result for the frame on the top of the stack */
        intermediate_result = IR_get(
            &parser,
            frame->production.rule->production,
            frame->backtrack_point->id
        );

        D( printf("intermediate result is %p. \n", (void *) intermediate_result); )

        /* get the current symbol that we are looking at */
        symbol = G_production_rule_get_symbol(
            frame->production.rule,
            frame->production.phrase,
            frame->production.symbol
        );

        /* an error occurred in the current frame. we need to pop it off, dump
         * its parse tree, backtrack, and possibly cascade the failure upward.
         */
        if(parser.must_backtrack) {

production_rule_failed:

            /* there are no rules to backtrack to, we need to cascade the
             * failure upward, dump the tree, and cache the failure.
             */
            j = G_production_rule_has_phrase(
                frame->production.rule,
                frame->production.phrase + 1
            );

            if(!j) {
                D( printf("cascading.\n"); )

                intermediate_result->intermediate_tree = IR_FAILED;

                /* pop the frame off of the stack */
                frame = parser.call.stack[j = --parser.call.frame];

                /* we cannot cascade off of the stack, therefore we encountered
                 * a parse error. */
                if(!parser.call.frame) {
                    D( printf("parse error.\n"); )
                    goto parse_error;
                }

                /* get the cached result for the calling frame */
                intermediate_result = IR_get(
                    &parser,
                    frame->production.rule->production,
                    frame->backtrack_point->id
                );

                /* if the top frame on the stack is being cascaded to and it has
                 * a left recursive seed then it hasn't also failed but its left
                 * recursive rule has succeeded. Given that, we must then move
                 * to the next symbol in the frame's current production rule
                 * phrase. */
                if(LR_intermediate_result_has_seed(intermediate_result)) {
                    parser.must_backtrack = 0;
                    frame->left_recursion.is_used = 0;
                    frame->production.symbol++;
                    intermediate_result->is_left_recursive = 0;
                }

                /* parser.must_backtrack remains set to 1 and so the cascade is
                 * automatic. */

            /* there is at least one rule to backtrack to. */
            } else {
                D( printf("backtracking.\n"); )

                parser.must_backtrack = 0;
                token = frame->backtrack_point;
                frame->production.phrase += 1;
            }

            continue;
        }

        /* a production successfully matched all of the non/terminals in its
         * current rule list. we need to pop it off, merge its parse tree into
         * the parent frame's (calling production) parse tree, and then tell
         * the parent frame to advance itself to the next non/terminal in its
         * current rule list. if there is no parent frame then we have
         * successfully parsed the tokens.
         */
        if(is_null(symbol)) {

production_rule_succeeded:

            if(parser.call.frame == 0) {

                /* there are tokens to parse but we have a single frame on
                 * on stack. this is a parse error, so we will backtrack. */
                if(token->id < parser.num_tokens) {
                    D( printf("no rules left. \n"); )
                    parser.must_backtrack = 1;

                /* we had a left recursive application, let's restart it to
                 * grow the left recursive seed further. */
                } else if(intermediate_result->is_left_recursive) {
                    D( printf("growing left recursion. \n"); )
                    token = LR_grow(&parser, intermediate_result, token);

                /* we have parsed all of the tokens. there is no need to do any
                 * work on the stack or the cache. */
                } else {
                    D( printf("done parsing.\n"); )
                    goto done_parsing;
                }

            /* we can't prove that this is a successful parse of all of the
             * tokens and so we assume that there exists more to parse. save
             * the result of the application of this production to the cache
             * and yield control to the parent frame.
             */
            } else {
                D( printf("production succeeded.\n"); )

                /* pop the production and advance to the next rule of the
                 * calling production. */
                F_update_success(
                    &parser,
                    (parser.call.frame)--,
                    intermediate_result
                );
            }

            continue;
        }

        /* We need to match the current token against a terminal symbol, accept
         * an epsilon symbol, or push a frame on for a non-terminal symbol. */
        if(token->id < parser.num_tokens) {

            /* the next non/terminal in the current rule list is a non-terminal,
             * i.e. it is a production. we need to push the production onto the
             * stack.
             */
            if(G_symbol_is_non_terminal(symbol)) {

match_non_terminal_symbol:

                intermediate_result = IR_get(
                    &parser,
                    symbol->value.non_terminal,
                    token->id
                );

                /* we have already applied this production to this particular
                 * place in the token stream. */
                if(is_not_null(intermediate_result)) {

                    /* the cached result is a failure. time to backtrack. */
                    if(IR_FAILED == intermediate_result->intermediate_tree) {
                        D( printf("cached production failed.\n"); )
                        parser.must_backtrack = 1;

                    /* the cached result is missing, i.e. the cache value was
                     * initialized by a production already on the stack but that
                     * has (in)directly called itself through left recursion. */
                    } else if(IR_INITIAL == intermediate_result->intermediate_tree) {

                        D( printf("detected left recursion. \n"); )

                        intermediate_result->is_left_recursive = 1;
                        intermediate_result->intermediate_tree = IR_FAILED;

                        D( printf("pushing production onto stack. \n"); )

                        frame = F_init(
                            F_push(&parser),
                            grammar->production_rules + symbol->value.non_terminal,
                            token
                        );

                        PTS_add(parser.trees, frame->parse_tree);
                        LR_mark_origin(&parser);

                    /* the cached result was a success, we need to merge the
                     * cached tree with our current parse tree, advance to the
                     * next rewrite rule in our current rule list, and advance
                     * our current position in the token list to the token after
                     * the last token that was matched in the cached result.
                     */
                    } else {
                        D( printf("cached production succeeded.\n"); )

                        F_record_tree(
                            frame,
                            intermediate_result->intermediate_tree
                        );

                        /* advance to a future token and to the next symbol */
                        ++(frame->production.symbol);
                        token = intermediate_result->end_token;
                    }

                /* we do not have a cached result and so we will need to push a
                 * new frame onto the stack.
                 */
                } else {
                    D( printf("pushing production onto stack.\n"); )

                    frame = F_init(
                        F_push(&parser),
                        grammar->production_rules + symbol->value.non_terminal,
                        token
                    );

                    PTS_add(parser.trees, frame->parse_tree);

                    IR_create(
                        &parser,
                        frame->production.rule->production,
                        token->id
                    );
                }

            /* the next non/terminal in the current rule list is a terminal,
             * i.e. it is a token. we need to try to match the current token
             * against the symbol's token. if they match then we need to advance
             * to the next rule and to the next token. if they do not match
             * then we need to signal the frame to backtrack.
             */
            } else if(G_symbol_is_terminal(symbol)) {

match_terminal_symbol:

                /* record the farthest point we've gotten to. */
                if(token->id > parser.farthest_id_reached) {
                    parser.farthest_id_reached = token->id;
                }

                /* we have matched a token, advance to the next token in the
                 * list and the next rewrite rule in the current rule list.
                 * also, store the matched token into the frame's partial parse
                 * tree. */
                if(token->terminal == symbol->value.terminal) {

                    D(
                        if(is_not_null(token->lexeme)) {
                            printf("\t matched: '%s' \n", token->lexeme->str);
                        } else {
                            printf("\t matched. \n");
                        }
                    )

                    /* store the match as a parse tree */
                    if(G_symbol_is_non_excludable(symbol)) {
                        tree_force_add_branch(
                            (PTree *) frame->parse_tree,
                            (PTree *) token
                        );
                    }

                    /* advance the token and phrase symbol */
                    token = token->next;
                    ++(frame->production.symbol);

                /* the tokens do not match, backtrack. */
                } else {
                    D(
                        printf(
                            "\t didn't match, expected:%d got:%d\n",
                            (unsigned int) symbol->value.terminal,
                            (unsigned int) token->terminal
                        );
                    )

                    parser.must_backtrack = 1;
                }

            /* the next non/terminal in the current rule list is an empty
             * terminal, i.e. it is the empty string. we accept it, move to the
             * next rule in the current rule list, but *don't* move to the next
             * token. */
            } else {

match_epsilon_symbol:

                D( printf("\t matched epsilon. \n"); )

                if(G_symbol_is_non_excludable(symbol)) {
                    temp_parse_tree = (PParseTree *) PT_alloc_epsilon();
                    PTS_add(parser.trees, temp_parse_tree);

                    tree_force_add_branch(
                        (PTree *) frame->parse_tree,
                        (PTree *) temp_parse_tree
                    );
                }

                ++(frame->production.symbol);
            }

            continue;
        }

        /* the parser stack has something on it but we've reached the end of
         * input, there are two cases to deal with:
         *   i) we need to match a series of epsilon transitions, which might
         *      lead to a successful parse. These epsilon transitions could be
         *      indirect (i.e. in productions) and so wee need to continue
         *      trying to match regardless of not having anything to match
         *      against.
         *   ii) we need to backtrack because what we're parsing is either
         *       missing information or the current derivation is such that
         *       we expect too much information. */
        if(token->id == parser.num_tokens) {

end_of_input:

            D( printf("end of input. \n"); )

            if(is_null(symbol) || G_symbol_is_terminal(symbol)) {
                parser.must_backtrack = 1;
            } else if(G_symbol_is_epsilon(symbol)) {
                goto match_epsilon_symbol;
            } else {
                goto match_non_terminal_symbol;
            }

            continue;
        }

        D( printf("wtf? \n"); )
    }

done_parsing:

    printf("Successfully parsed. \n");

    /* go over our reduced parse tree and remove any references to trees
     * listed in our all trees hash table to that we can free all of the
     * remaining trees in there at once. */
    temp_parse_tree = (PParseTree *) frame->parse_tree;
    tree_generator = tree_generator_alloc(
        (PTree *) temp_parse_tree,
        TREE_TRAVERSE_POSTORDER
    );
    while(generator_next(tree_generator)) {
        PTS_remove(parser.trees, generator_current(tree_generator));
    }
    generator_free(tree_generator);

    goto clean_parser;

parse_error:

    temp_parse_tree = NULL;
    printf("A parse error occurred. \n");

clean_parser:

    D( printf("freeing resources... \n"); )
    PTS_free(parser.trees);
    D( printf("garbage trees freed, freeing intermediate results... \n"); )
    IR_free_all(&parser, grammar);
    D( printf("intermediate results freed, freeing parser stack... \n"); )
    F_free_all(&parser);
    D( printf("parser stack freed. \n"); )


    PARSER_DEBUG(printf("returning parse tree...\n");)

    return temp_parse_tree;
}
