/*
 * p-parser.c
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-parser.h>

#define D(x) x

#define PT_ENABLE_TREE_REDUCTIONS 1

#define IR_FAILED ((void *) 10)
#define IR_INITIAL ((void *) 11)

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
 * Add a branch to a frame's parse tree.
 */
static void F_add_branch(P_Frame *frame,
                         PParseTree *branch_tree,
                         G_Symbol *symbol) {
#if PT_ENABLE_TREE_REDUCTIONS
    unsigned short num_branches = tree_get_num_branches((PTree *) branch_tree);

    if(!(symbol->is_non_excludable)
    && num_branches <= 1
    && branch_tree->type == PT_NON_TERMINAL) {

        /* this is a production with only one child filled,
         * promote that single child node to the place of this
         * production in the tree and ignore that production. */
        if(num_branches == 1) {
            tree_force_add_branch(
                (PTree *) frame->parse_tree,
                tree_get_branch((PTree *) branch_tree, 0)
            );
        }

    /* this node must be added to the tree, has more than one child, or must
     * have its children included in the parse tree. */
    } else {
        if(symbol->children_must_be_raised) {
            tree_force_add_branch_children(
                (PTree *) frame->parse_tree,
                (PTree *) branch_tree
            );
        } else {
            tree_force_add_branch(
                (PTree *) frame->parse_tree,
                (PTree *) branch_tree
            );
        }
    }
#else
    tree_force_add_branch(
        (PTree *) frame->parse_tree,
        (PTree *) branch_tree
    );
#endif
    return;
}

/**
 * Update the tree of one of the stack frames.
 */
static void F_record_tree(P_Frame *frame, PParseTree *parse_tree) {
    F_add_branch(
        frame,
        parse_tree,
        G_production_rule_get_symbol(
            frame->production.rule,
            frame->production.phrase,
            frame->production.symbol
        )
    );
}

/**
 * Create the initial intermediate result.
 */
static P_IntermediateResult *IR_create(PParser *parser, G_NonTerminal production, uint32_t id) {

    P_IntermediateResult *result;
    uint32_t i = ((uint32_t) production * (parser->num_tokens + 1)) + (uint32_t) id;

    assert(id <= parser->num_tokens);

    if(is_null(parser->intermediate_results[i])) {

        result = mem_alloc(sizeof(P_IntermediateResult));
        if(is_null(result)) {
            mem_error("Unable to allocate new intermediate result.");
        }

        result->end_token = NULL;
        result->intermediate_tree = IR_INITIAL;
        result->uses_indirect_left_recursion = 0;
        result->is_being_retested = 0;

        parser->intermediate_results[i] = result;

        return result;

    } else {
        return parser->intermediate_results[i];
    }
}

/**
 * Push a new stack frame onto the parser's frame stack and initialize that
 * frame.
 */
static P_Frame *F_push(PParser *parser,
                       G_ProductionRule *rule,
                       PT_Terminal *backtrack_point) {

    P_Frame *frame;
    unsigned int i = ++(parser->call.frame);

    /* TODO better error here */
    if(i >= P_MAX_RECURSION_DEPTH) {
        std_error("Internal Parse Error: maximum recursion depth exceeded.\n");
    }

    /* only allocate a new frame if we need to, otherwise re-use an existing
     * one. */
    if(is_null(parser->call.stack[i])) {
        parser->call.stack[i] = mem_alloc(sizeof(P_Frame));
        if(is_null(parser->call.stack[i])) {
            mem_error("Unable to allocate new parser frame.");
        }
    }

    /* initialize the frame */
    frame = parser->call.stack[i];

    frame->backtrack_point = backtrack_point;

    frame->left_recursion.is_direct = 0;
    frame->left_recursion.is_used = 0;
    frame->left_recursion.symbol = NULL;

    frame->production.phrase = 0;
    frame->production.symbol = 0;
    frame->production.rule = rule;
    frame->production.is_committed = 0;

    frame->parse_tree = (PParseTree *) PT_alloc_non_terminal(
        rule->production,
        2 /* TODO: start bigger? */
    );

    PTS_add(parser->tree_set, frame->parse_tree);

    IR_create(
        parser,
        rule->production,
        backtrack_point->id
    )->end_token = backtrack_point;

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
             j_max = parser->num_tokens+1,
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
 * Clear the branches off of an intermediate parse tree.
 */
static void IR_clear_tree(P_IntermediateResult *result) {
    PTree *tree = (PTree *) result->intermediate_tree;

    if(is_not_null(tree) && IR_FAILED != tree && IR_INITIAL != tree) {
        tree_clear(tree);
    }
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
        [((uint32_t) production * (parser->num_tokens + 1)) + (uint32_t) id]
    );
}

/* -------------------------------------------------------------------------- */

/**
 * Search the parser stack for the production that originated the left
 * recursive rule on the top of the parser stack.
 */
static void LR_mark_origin(PParser *parser, P_IntermediateResult *result) {

    int i = parser->call.frame;

    P_Frame *top = parser->call.stack[i],
            *origin;

    for(; i > 0; ) {

        origin = parser->call.stack[--i];

        if(origin->production.rule->production
        == top->production.rule->production) {

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

    result->uses_indirect_left_recursion = !origin->left_recursion.is_direct;
}

/* -------------------------------------------------------------------------- */

static void P_perform_production_rule_actions(PGrammar *grammar,
                                              PParseTree *tree,
                                              void *state,
                                              PTreeTraversalType tree_taversal_type) {
    PTreeGenerator *tree_generator = NULL;
    PT_NonTerminal *non_terminal;

    /* go over our reduced parse tree and remove any references to trees
     * listed in our all trees hash table to that we can free all of the
     * remaining trees in there at once. */
    tree_generator = tree_generator_alloc(
        (PTree *) tree,
        tree_taversal_type
    );

    while(generator_next(tree_generator)) {
        tree = generator_current(tree_generator);
        if(tree->type == PT_NON_TERMINAL) {
            non_terminal = (PT_NonTerminal *) tree;
            (grammar->production_rules + non_terminal->production)->action_fnc(
                state,
                non_terminal->phrase,
                tree_get_num_branches((PTree *) non_terminal),
                (PParseTree **) (((PTree *) non_terminal)->_branches)
            );
        }
    }

    generator_free(tree_generator);
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
void parse_tokens(PGrammar *grammar,
                  PScanner *scanner,
                  PScannerFunction *scanner_fnc,
                  void *state,
                  PTreeTraversalType tree_taversal_type) {

    PParser parser;
    P_Frame *frame = NULL,
            *caller = NULL;
    P_IntermediateResult *intermediate_result = NULL,
                         *temp_result = NULL;
    G_Symbol *symbol = NULL,
             *next_symbol = NULL;
    PParseTree *temp_parse_tree = NULL;
    PT_Terminal *token = NULL;

    /* useful counter, be it for hinting to GCC that a block shouldn't be
     * optimized out, or for actually counting something. */
    unsigned int j = 0,
                 delt_with_end_of_input = 0;

    D( printf("initializing parser...\n"); )

    parser.tree_set = PTS_alloc();

    D( printf("accumulating tokens... \n"); )

    /* get all of the tokens as terminal trees */
    token = PT_alloc_terminals(
        scanner,
        scanner_fnc,
        parser.tree_set
    );

    printf("first token %p \n", (void *) token);

    D( printf("%d tokens in memory, configuring... \n", PTS_size(parser.tree_set)); )

    /* minus one because we add in a token for EOF */
    parser.num_tokens = PTS_size(parser.tree_set) - 1;
    parser.farthest_id_reached = 0;
    parser.call.frame = -1;
    parser.must_backtrack = 0;

    /*
    if(parser.num_tokens == 0) {
        parse_tree_free((PParseTree *) token);
        goto clean_the_rest;
    }*/

    G_lock(grammar);

    /* allocate the cache table */
    parser.intermediate_results = mem_calloc(
        (parser.num_tokens + 1) * grammar->num_productions,
        sizeof(P_IntermediateResult *)
    );

    /* clear out the frame stack */
    for(j = 0; j < P_MAX_RECURSION_DEPTH; ++j) {
        parser.call.stack[j] = NULL;
    }

    /* get the starting production and push our first stack frame on. This
     * involves registering the start of the token list as the farthest back
     * we can backtrack.
     */
    D( printf("pushing production onto stack...\n"); )

    /* push on our first stack frame */
    frame = F_push(
        &parser,
        grammar->production_rules + grammar->start_production_rule,
        token
    );

    /*PTS_add(parser.tree_set, frame->parse_tree);*/
    /*IR_create(&parser, frame->production.rule->production, 0);*/

    D( printf("beginning main parse...\n"); )

    while(0 <= parser.call.frame) {

parser_begin_loop:

        frame = parser.call.stack[j = parser.call.frame];

        D( printf("\nframe is %p, phrase is %d, symbol is %d, at %d \n", (void *) frame, frame->production.phrase, frame->production.symbol, (int) parser.call.frame); )

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

        D( printf("symbol is %p. \n", (void *) symbol); )

        /* an error occurred in the current frame. we need to pop it off, dump
         * its parse tree, backtrack, and possibly cascade the failure upward.
         */
        if(parser.must_backtrack) {

backtrack_raised:

            /* make (mostly) sure that a phrase isn't found */
            if(frame->production.is_committed) {
                frame->production.phrase = (unsigned char) -1;
                D( printf("Forcing production to commit. \n"); )
            }

            /* there are no rules to backtrack to, we need to cascade the
             * failure upward, dump the tree, and cache the failure.
             */
            j = G_production_rule_has_phrase(
                frame->production.rule,
                frame->production.phrase + 1
            );

            if(!j) {

                /* trying to cascade off of the stack */
                if(parser.call.frame == 0) {
                    D( printf("parse error.\n"); )
                    goto parse_error;
                }

                caller = parser.call.stack[j = parser.call.frame - 1];
                temp_result = IR_get(
                    &parser,
                    caller->production.rule->production,
                    caller->backtrack_point->id
                );

                /* the next frame is left recursive */
                if(caller->left_recursion.is_used
                && intermediate_result->intermediate_tree != IR_FAILED
                && temp_result->intermediate_tree != IR_FAILED) {

stop_left_recursion:

                    parser.must_backtrack = 0;
                    token = temp_result->end_token;



                    if(caller->left_recursion.is_direct) {

                        --(parser.call.frame);
                        caller->left_recursion.is_used = 0;
                        caller->parse_tree = temp_result->intermediate_tree;
                        ++(caller->production.symbol);

                    } else {
                        frame->parse_tree = (
                            intermediate_result->intermediate_tree
                        );

                        goto phrase_completed;
                    }



                } else {

cascading_backtrack:

                    D( printf("cascading.\n"); )

                    --(parser.call.frame);
                    intermediate_result->intermediate_tree = IR_FAILED;

                    /* parser.must_backtrack remains set to 1 and so the cascade is
                     * automatic. */

                }

            /* there is at least one rule to backtrack to. */
            } else {

local_backtrack:

                D( printf("backtracking.\n"); )

                parser.must_backtrack = 0;

                token = frame->backtrack_point;

                ++(frame->production.phrase);
                ++(((PT_NonTerminal *) frame->parse_tree)->phrase);
                frame->production.symbol = 0;

                /* drop the branches of the parse tree */
                tree_clear((PTree *) frame->parse_tree);

                D( printf("new token is %p=%d %d (backtrack). \n", (void *) token, token->id, frame->backtrack_point->id); )
            }

        /* a production successfully matched all of the non/terminals in its
         * current rule list. we need to pop it off, merge its parse tree into
         * the parent frame's (calling production) parse tree, and then tell
         * the parent frame to advance itself to the next non/terminal in its
         * current rule list. if there is no parent frame then we have
         * successfully parsed the tokens.
         */
        } else if(is_null(symbol)) {

phrase_completed:

            /* there are tokens to parse but we have a single frame on
             * on stack. this is a parse error, so we will backtrack. */
            if(parser.call.frame == 0 && token->id < parser.num_tokens) {

                D( printf("no rules left to parse remaining tokens. \n"); )
                parser.must_backtrack = 1;

            /* we have parsed all of the tokens. there is no need to do any
             * work on the stack or the cache. */
            } else if(parser.call.frame == 0) {

                D( printf("done parsing.\n"); )
                goto done_parsing;

            /* we've fully matched a production rule  */
            } else {

                D( printf("production succeeded.\n"); )

                D( printf(
                    "token id %d, IR->fpr %d ***\n",
                    token->id,
                    intermediate_result->end_token->id
                ); )

                caller = parser.call.stack[parser.call.frame - 1];
                intermediate_result->is_being_retested = 0;

                /* the production rule that we matched is the origin of left
                 * recursion and its most recent application was successful. */
                if(frame->left_recursion.is_used
                && token->id > intermediate_result->end_token->id) {

grow_left_recursion:

                    D( printf("\t growing left-recursion.\n"); )

                    intermediate_result->intermediate_tree = frame->parse_tree;
                    intermediate_result->end_token = token;

                    frame->parse_tree = (PParseTree *) PT_alloc_non_terminal(
                        frame->production.rule->production,
                        2 /* TODO: start bigger? */
                    );

                    PTS_add(parser.tree_set, frame->parse_tree);

                    /* re-start this frame */
                    token = frame->backtrack_point;
                    frame->production.phrase = 0;
                    frame->production.symbol = 0;

                    if(!frame->left_recursion.is_direct) {
                        intermediate_result->is_being_retested = 1;
                    }

                /* pop the production and advance to the next rule of the
                 * calling production. */
                } else {

pop_production_rule:

                    D( printf("\t popping frame off of stack. \n"); )

                    intermediate_result->end_token = token;
                    intermediate_result->intermediate_tree = frame->parse_tree;
                    intermediate_result->is_being_retested = 0;

                    frame = parser.call.stack[j = --(parser.call.frame)];
                    F_record_tree(frame, intermediate_result->intermediate_tree);

                    ++(frame->production.symbol);
                }
            }

        } else if(token->id >= parser.num_tokens && symbol->is_terminal) {
end_of_input:

            D( printf("end of input with more tokens to parse, backtracking. \n"); )
            parser.must_backtrack = 1;

        } else {

            /* accept any number of consecutive symbols */
            while(symbol->is_cut) {

match_cut_symbol:

                D( printf("committing to phrase. \n"); )

                frame->production.is_committed = 1;

                symbol = G_production_rule_get_symbol(
                    frame->production.rule,
                    frame->production.phrase,
                    ++(frame->production.symbol)
                );

                if(is_null(symbol)) {
                    goto parser_begin_loop;
                }
            }

            if(symbol->is_fail) {

match_fail_symbol:
                parser.must_backtrack = 1;

            /* the next non/terminal in the current rule list is a non-terminal,
             * i.e. it is a production. we need to push the production onto the
             * stack.
             */
            } else if(symbol->is_non_terminal) {

                D( printf("non-terminal symbol found. \n"); )

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

                        intermediate_result->intermediate_tree = IR_FAILED;

                        D( printf("pushing production onto stack. \n"); )

                        frame = F_push(
                            &parser,
                            grammar->production_rules + symbol->value.non_terminal,
                            token
                        );

                        LR_mark_origin(&parser, intermediate_result);

                    /* indirect left recursion is special in that the production
                     * rule used to get at the left-recursive call will have a
                     * cached version that is not up-to-date with actual indirect
                     * left-recursive call. To force the cache to update itself
                     * we need to push the first
                     */
                    } else if(intermediate_result->uses_indirect_left_recursion
                    && !intermediate_result->is_being_retested
                    && parser.call.frame > 0) {

                        caller = parser.call.stack[parser.call.frame - 1];

                        /* make sure that we are dealing with left recursion or
                         * something equivalent to it. */
                        if(caller->backtrack_point == token) {

                            D( printf("\t pushing on out-of-date cached result \n"); )

                            intermediate_result->is_being_retested = 1;

                            frame = F_push(
                                &parser,
                                grammar->production_rules + symbol->value.non_terminal,
                                token
                            );

                        } else {
                            goto cached_production_succeeded;
                        }

                    /* the cached result was a success, we need to merge the
                     * cached tree with our current parse tree, advance to the
                     * next rewrite rule in our current rule list, and advance
                     * our current position in the token list to the token after
                     * the last token that was matched in the cached result.
                     */
                    } else {

cached_production_succeeded:

                        D( printf("cached production succeeded.\n"); )

                        F_add_branch(
                            frame,
                            intermediate_result->intermediate_tree,
                            symbol
                        );

                        /* advance to a future token and to the next symbol */
                        ++(frame->production.symbol);

                        token = intermediate_result->end_token;

                        D( printf("new token is %p=%d (cached production). \n", (void *) token, token->id); )
                    }

                /* we do not have a cached result and so we will need to push a
                 * new frame onto the stack. */
                } else {
                    D( printf("pushing production onto stack.\n"); )

                    next_symbol = G_production_rule_get_symbol(
                        frame->production.rule,
                        frame->production.phrase,
                        frame->production.symbol + 1
                    );

                    frame = F_push(
                        &parser,
                        grammar->production_rules + symbol->value.non_terminal,
                        token
                    );
                }

            /* the next non/terminal in the current rule list is a terminal,
             * i.e. it is a token. we need to try to match the current token
             * against the symbol's token. if they match then we need to advance
             * to the next rule and to the next token. if they do not match
             * then we need to signal the frame to backtrack.
             */
            } else if(symbol->is_terminal) {

match_terminal_symbol:

                D( printf("terminal symbol found. \n"); )

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
                            printf("\t matched: '%s' on line %d, column %d. \n", token->lexeme->str, token->line, token->column);
                        } else {
                            printf("\t matched. \n");
                        }
                    )

                    /* store the match as a parse tree */
                    if(symbol->is_non_excludable || !PT_ENABLE_TREE_REDUCTIONS) {
                        F_add_branch(frame, (PParseTree *) token, symbol);
                    }

                    /* advance the token and phrase symbol */
                    token = token->next;

                    D( printf("new token is %p=%d (next). \n", (void *) token, token->id); )

                    ++(frame->production.symbol);

                /* the tokens do not match, backtrack. */
                } else {

                    D(
                        printf(
                            "\t didn't match, expected:%d got:%d\n",
                            (unsigned int) symbol->value.terminal,
                            (unsigned int) token->terminal
                        );

                        if(is_not_null(token->lexeme)) {
                            printf("\t lexeme is '%s', token->id is %d. \n", token->lexeme->str, token->id);
                        } else {
                            printf("\t token->id is %d. \n", token->id);
                        }
                    )

                    parser.must_backtrack = 1;
                }

            /* accept any number of consecutive epsilon transitions (match the
             * empty string). */
            } else if(symbol->is_epsilon_transition) {

match_epsilon_symbol:

                D( printf("\t epsilon symbol found. \n"); )

                do {
                    if(symbol->is_non_excludable || !PT_ENABLE_TREE_REDUCTIONS) {
                        temp_parse_tree = (PParseTree *) PT_alloc_epsilon();
                        PTS_add(parser.tree_set, temp_parse_tree);
                        F_add_branch(frame, temp_parse_tree, symbol);
                    }

                    /* get the next symbol */
                    symbol = G_production_rule_get_symbol(
                        frame->production.rule,
                        frame->production.phrase,
                        ++(frame->production.symbol)
                    );

                } while(is_not_null(symbol) && symbol->is_epsilon_transition);
            } else {
                std_error("Internal Parser Error: Strange conditions met.");
            }
        }
    }

parse_error:

    temp_parse_tree = NULL;
    D( printf("A parse error occurred. \n"); )
    goto return_from_parser;

done_parsing:

    D( printf("\n\nSuccessfully parsed. \n"); )

    P_perform_production_rule_actions(
        grammar,
        (PParseTree *) frame->parse_tree,
        state,
        tree_taversal_type
    );

    G_unlock(grammar);

clean_intermediate_results:

    D( printf("freeing resources... \n"); )
    IR_free_all(&parser, grammar);

clean_the_rest:

    D( printf("intermediate results freed, freeing parser stack... \n"); )
    PTS_free(parser.tree_set);
    D( printf("garbage trees freed, freeing intermediate results... \n"); )
    F_free_all(&parser);
    D( printf("parser stack freed. \n"); )

    D(printf("returning parse tree...\n");)

return_from_parser:

    return;
}
