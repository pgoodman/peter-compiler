/*
 * p-parser.c
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-parser.h>

#define P_SIZE_OF_THUNK (0 \
         + sizeof(PParserFunc) \
         + sizeof(PGenericList *) \
        ) / sizeof(char)
#define P_PRODUCTION_FAILED ((void *) 10)
#define P_PRODUCTION_TRY_LR ((void *) 11)
#define P_SIZE_OF_PARSE_TREE (sizeof(PParseTree *) / sizeof(char))
#define PARSER_DEBUG(x) x


/* Type representing the necessary information to uniquely identify the result
 * of a production after being applied to a particular suffix of tokens. In this
 * case, list is some pointer to a generic list of tokens that the production was
 * applied to. The token list is ordered and never changes order and so the
 * result of a production to some part in the token list can be meaningfully
 * cached.
 */
typedef struct P_Thunk {
    PParserFunc production;
    PTerminalTree *list;
} P_Thunk;

/**
 * Type representing the cached result of the application of a production to
 * some part in the token list 'start'. This type is only used on successful
 * parse, the P_PRODUCTION_FAILED pointer is instead cached to indicate a failed
 * application of a production to some part of the token list.
 *
 * 'end' points to somewhere in the token list for the parser to pick up after
 * accepting the cached parse tree, 'tree', from the application of this
 * production.
 */
typedef struct P_CachedResult {
    PTerminalTree *end;
    PParseTree *tree;
    char is_left_recursive;
} P_CachedResult;

/**
 * Type holding a pointer to the ordered list of *all* tokens from whatever text
 * is currently being parsed, as well as the number of tokens in that list. Right
 * off the bat we allocate the leaves of the parse tree and put the tokens in
 * them so that no tree leaf for a given token is allocated more than once. This
 * also gives us the benefit of having a well-defined in-order traversal path in
 * the final parse tree.
 */
typedef struct P_TerminalTreeList {
    PTerminalTree *list;
    uint32_t num_tokens;
} P_TerminalTreeList;

/**
 * In a sense this parser is recursive-descent; however, it maintains its own
 * heap-allocated stack frames, as opposed to calling production functions
 * recursively.
 *
 * The choice to manage stack frames explicitly has a few very nice benefits:
 *   i) it allows for the functions that perform the actual parsing to be
 *      isolated and for the operations of the parser to be clearly defined and
 *      predictable.
 *   ii) it allows for the actual parser to be defined entirely in terms of
 *       primitive pattern matching against the grammar as opposed to requiring
 *       independent functions to be constructed to perform the actual parsing.
 *   iii) it allows for a one-to-one correspondence between the definition of
 *        the parser in C and the top-down parsing language grammar.
 *
 * A stack frame holds important information, namely:
 *    a) the current production in being executed.
 *    b) a list of the remaining non/terminals to match against tokens in the
 *       token list. When this list is NULL it implies that we have reached the
 *       end of it and thus have matched all of the non/terminals of this
 *       particular production's rule.
 *    c) the alternative rules if any non/terminal in the curr_rule_list fails
 *       to match. If the alternative_rules list is null and the 'do_backtrack'
 *       flag is set then it implies that all of the rules for the current
 *       production have failed to match the tokens.
 *    d) the backtrack_point points to where in the token list the parser was
 *       when it attempted to start matching this production. This is the point
 *       where it will return to on failure in order to try matching another
 *       rule.
 *    e) a do_backtrack flag indicating that a non/terminal in
 *       curr_rule_list has failed to match a token and so we must
 *       backtrack to the backtrack_point and try another rule, or upon
 *       full failure of the production, cascade the failure to the
 *       calling production.
 *    f) the parse tree as it is being constructed.
 *    g) the calling stack frame. This is used for two reasons, it lets us
 *       manually manage the call stack without the overhead of the generic
 *       stack data structure and it also lets us reclaim stack frames for
 *       future use.
 */
typedef struct P_StackFrame {
    P_Production *production;
    PGenericList *curr_rule_list,
                 *alternative_rules;
    PTerminalTree *backtrack_point;
    char do_backtrack;
    PParseTree *parse_tree;
    struct P_StackFrame *caller;
} P_StackFrame;

/**
 * Hash function that converts a thunk to a char array. Thunks are used as the
 * keys into a hash table for cached parse results.
 */
static uint32_t P_thunk_hash_fnc(void *pointer) {
    union {
        P_Thunk thunk;
        char thunk_as_chars[P_SIZE_OF_THUNK];
    } switcher;
    switcher.thunk = *((P_Thunk *) pointer);
    return murmur_hash(switcher.thunk_as_chars, P_SIZE_OF_THUNK, 73);
}

/**
 * Check for thunk collisions.
 */
static char P_thunk_collision_fnc(void *thunk1, void *thunk2) {
    P_Thunk *t1 = thunk1,
            *t2 = thunk2;
    return ((t1->list != t2->list) || (t1->production != t2->production));
}

/**
 * Allocate a new cached result of parsing on the heap.
 */
static P_CachedResult *P_alloc_cache(PTerminalTree *end, PParseTree *tree) {

    P_CachedResult *R = mem_alloc(sizeof(P_CachedResult));
    if(is_null(R)) {
        mem_error("Unable to cache the result of a production on the heap.");
    }

    R->end = end;
    R->tree = tree;
    R->is_left_recursive = 0;

    return R;
}

/**
 * Allocate a new thunk on the heap.
 */
static P_Thunk *P_alloc_thunk(PParserFunc func, PTerminalTree *list) {
    P_Thunk *thunk = mem_alloc(sizeof(P_Thunk));

    if(is_null(thunk)) {
        mem_error("Unable to heap allocate a thunk.");
    }

    thunk->list = list;
    thunk->production = func;

    return thunk;
}

/**
 * Allocate a new production stack frame on the heap or re-use a previously
 * allocated but now unused one.
 */
static P_StackFrame *P_frame_stack_alloc(P_StackFrame **unused,
                                         P_Production *prod,
                                         PTerminalTree *backtrack_point,
                                         PDictionary *all_trees,
                                         char record_tree) {
    P_StackFrame *frame = NULL;
    P_Thunk thunk;

    if(is_null(*unused)) {
        frame = mem_alloc(sizeof(P_StackFrame));
        if(is_null(frame)) {
            mem_error("Unable to allocate new parser stack frame on the heap.");
        }
    } else {
        frame = (*unused);
        (*unused) = frame->caller;
    }

    frame->caller = NULL;
    frame->production = prod;
    frame->curr_rule_list = gen_list_get_elm(prod->alternatives);
    frame->alternative_rules = (PGenericList *) list_get_next(prod->alternatives);
    frame->backtrack_point = backtrack_point;
    frame->do_backtrack = 0;
    frame->parse_tree = NULL;

    /* make the parse tree */
    if(record_tree) {

        frame->parse_tree = tree_alloc(
            sizeof(PProductionTree),
            (unsigned short) prod->max_rule_elms
        );

        frame->parse_tree->type = P_PARSE_TREE_PRODUCTION;
        ((PProductionTree *) (frame->parse_tree))->rule = 1;
        ((PProductionTree *) (frame->parse_tree))->production = prod->production;

        dict_set(
            all_trees, frame->parse_tree,
            frame->parse_tree, &delegate_do_nothing
        );
    }

    /* record the cache entry for this frame */
    thunk.list = backtrack_point;
    thunk.production = prod->production;

    return frame;
}

/**
 * Push a stack frame onto the frame stack.
 */
static void P_frame_stack_push(P_StackFrame **stack, P_StackFrame *frame) {
    frame->caller = (*stack);
    (*stack) = frame;
    return;
}

/**
 * Pop a stack frame off of the frame stack and put it into the unused stack
 * where it will still be accessible.
 */
static void P_frame_stack_pop(P_StackFrame **unused, P_StackFrame **stack) {
    P_StackFrame *frame = (*stack);

    (*stack) = frame->caller;
    frame->caller = (*unused);
    (*unused) = frame;

    /*
    frame->alternative_rules = NULL;
    frame->backtrack_point = NULL;
    frame->caller = NULL;
    frame->curr_rule_list = NULL;
    frame->do_backtrack = 0;
    frame->parse_tree = NULL;
    frame->production = NULL;
    */

    return;
}

/**
 * Allocate a new terminal tree.
 */
static PTerminalTree *P_alloc_terminal_tree(PToken *tok, PDictionary *all_trees) {
    PTerminalTree *T = NULL;

    assert_not_null(tok);
    assert_not_null(all_trees);

    T = mem_alloc(sizeof(PTerminalTree));
    if(is_null(T)) {
        mem_error("Unable to allocate parse tree leaf node.");
    }

    T->token = tok;
    T->next = NULL;
    T->prev = NULL;
    ((PParseTree *) T)->type = P_PARSE_TREE_TERMINAL;

    dict_set(all_trees, T, T, &delegate_do_nothing);

    return T;
}

/**
 * Return a linked list of all tokens in the current file being parsed.
 */
static P_TerminalTreeList P_get_all_tokens(PTokenGenerator *G, PDictionary *D) {

    P_TerminalTreeList list;
    PTerminalTree *prev = NULL,
                  *curr = NULL;

    assert_not_null(G);
    assert_not_null(D);

    list.list = NULL;
    list.num_tokens = 0;

    /* build up a list of all of the tokens. */
    if(!generator_next(G)) {
        return list;
    }

    /* generate the first token */
    list.list = P_alloc_terminal_tree(generator_current(G), D);
    prev = list.list;
    list.num_tokens = 1;

    /*printf("'%s'\n", (list.list)->token->val->str);*/

    /* generate the rest of the tokens */
    while(generator_next(G)) {
        ++(list.num_tokens);
        curr = P_alloc_terminal_tree(generator_current(G), D);
        prev->next = curr;
        curr->prev = prev;
        prev = curr;

        /*printf("'%s'\n", curr->token->val->str);*/
    }
    file_free(G->stream);

    return list;
}

/**
 * Parse the tokens from a token generator. This parser operates on a simplified
 * TDPL (Top-Down Parsing Language). It supports *local* backtracking. As such,
 * this parser cannot be used to parse ambiguous because expression rules in
 * productions are ordered and once one succeeds for a particular input the
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
 * succeeds then we cache its parse treee, where in the token list it started
 * and ended, and the production function itself.
 *
 * TODO: i) implement useful parse error reporting and recovery
 *       ii) allow for left-recursion, as described in the following article:
 *           http://portal.acm.org/citation.cfm?id=1328408.1328424
 */
void parser_parse_tokens(PParser *P, PTokenGenerator *G) {
    PToken *curr_token = NULL;
    P_Production *curr_production = NULL;
    P_TerminalTreeList token_result;
    PParseTree *parse_tree = NULL;
    PParserRewriteRule *curr_rule;
    P_StackFrame *frame = NULL,
                 *unused = NULL;
    PTerminalTree *curr = NULL;
    P_CachedResult *cached_result = NULL;
    P_Thunk thunk;
    PDictionary *thunk_table = NULL,
                *all_parse_trees = NULL;
    PTreeGenerator *gen = NULL;
    char record_tree = 1;

    /* stats */
    int num_tok_comparisons = 0,
        num_func_calls = 0,
        num_cache_uses = 0,
        num_backtracks = 0,
        num_cached_successes = 0,
        num_cached_failures = 0,
        j = 0;

    /* for parse error info, the farthest point reached in the token stream. */
    struct {
        uint32_t line;
        uint32_t column;
    } fpr;

    assert_not_null(P);
    assert_not_null(G);
    assert(prod_dict_is_set(P->productions, P->start_production));

    /* error reporting info */
    fpr.column = 0;
    fpr.line = 0;

    /* set for all trees to go in. while construction the AST from the CST we
     * will be removing some nodes and keeping others. We use this dictionary
     * to first track *all* trees, then we will remove the trees that we are
     * _keeping_ in the AST from the dictionary so that all the remaining trees
     * in the set can be freed.
     */
    all_parse_trees = dict_alloc(
        53,
        &dict_pointer_hash_fnc,
        &dict_pointer_collision_fnc
    );

    token_result = P_get_all_tokens(G, all_parse_trees);

    /* close the parser to updates on its grammar. */
    P->is_closed = 1;

    /* no tokens were lexed,
     * TODO: do something slightly more useful. */
    if(0 == token_result.num_tokens) {
        /*return parse_tree;*/
        return;
    }

    curr = token_result.list;

    /* allocate enough space for a single perfect pass through the text for
     * thunks.
     */
    thunk_table = dict_alloc(
        token_result.num_tokens,
        &P_thunk_hash_fnc,
        &P_thunk_collision_fnc
    );

    /* get the starting production and push our first stack frame on. This
     * involves registering the start of the token list as the furthest back
     * we can backtrack.
     */
    curr_production = prod_dict_get(P->productions, P->start_production);

    PARSER_DEBUG(printf("pushing production onto stack.\n");)
    P_frame_stack_push(
        &frame,
        P_frame_stack_alloc(
            &unused, curr_production,
            curr, all_parse_trees, record_tree
        )
    );

    parse_tree = frame->parse_tree;

    while(is_not_null(frame)) {

        /*PARSER_DEBUG(if(++j > 400) break;)*/

        /* get the rewrite rule and the current token we are looking at. */
        curr_production = frame->production;

        /* an error occurred in the current frame. we need to pop it off, dump
         * its parse tree, backtrack, and possibly cascade the failure upward.
         */
        if(frame->do_backtrack) {

            ++num_backtracks;

            /* clear off the branches of the parse tree */
            if(record_tree) {
                tree_clear(frame->parse_tree);
            }

            /* there are no rules to backtrack to, we need to cascade the
             * failure upward, dump the tree, and cache the failure.
             */
            if(is_null(frame->alternative_rules)) {

                PARSER_DEBUG(printf("cascading.\n");)

                /* dump the reference to the parse tree */
                if(record_tree) {
                    frame->parse_tree = NULL;
                }

                ++num_cached_failures;

                /* cache the failure */
                dict_set(
                   thunk_table,
                   P_alloc_thunk(
                       curr_production->production,
                       frame->backtrack_point
                   ),
                   P_PRODUCTION_FAILED,
                   &delegate_do_nothing
                );

                /* pop the frame */
                P_frame_stack_pop(&unused, &frame);

                /* check if we encountered a parse error. */
                if(is_null(frame)) {
                    PARSER_DEBUG(printf("parse error.\n");)
                    break;
                }

                /* the the frame on the top of the stack (callee) to backtrack. */
                frame->do_backtrack = 1;

            /* there is at least one rule to backtrack to. */
            } else {

                PARSER_DEBUG(printf("backtracking.\n");)

                /* switch rules */
                frame->curr_rule_list = (PGenericList *) gen_list_get_elm(
                    frame->alternative_rules
                );
                frame->alternative_rules = (PGenericList *) list_get_next(
                    frame->alternative_rules
                );

                /* backtrack to where this production tried to start matching
                 * from.
                 */
                curr = frame->backtrack_point;

                /* don't cascade */
                frame->do_backtrack = 0;
            }

        /* a production successfully matched all of the non/terminals in its
         * current rule list. we need to pop it off, merge its parse tree into
         * the parent frame's (calling production) parse tree, and then tell
         * the parent frame to advance itself to the next non/terminal in its
         * current rule list. if there is no parent frame then we have
         * successfully parsed the tokens.
         */
        } else if(is_null(frame->curr_rule_list)) {


            if(is_null(frame->caller)) {

                /* there are no tokens to parse but we have a single frame on
                 * on stack. this is a parse error, so we will backtrack. */
                if(is_not_null(curr)) {

                    printf("here %s line %d col %d.\n", curr->token->val->str, curr->token->line, curr->token->column);
                    frame->do_backtrack = 1;

                /* we have parsed all of the tokens. there is no need to do any
                 * work on the stack or the cache. */
                } else {
                    PARSER_DEBUG(printf("done parsing.\n");)
                    j++; /* make GCC not optimize out this block. */
                    break;
                }

            /* we can't prove that this is a successful parse of all of the
             * tokens and so we assume that there exists more to parse. save
             * the result of the application of this production to the cache
             * and yield control to the parent frame.
             */
            } else {

                PARSER_DEBUG(printf("production succeeded.\n");)

                ++num_cached_successes;

                /* cache the successful application of this production. */
                dict_set(
                    thunk_table,
                    P_alloc_thunk(
                        curr_production->production,
                        frame->backtrack_point
                    ),
                    P_alloc_cache(
                        curr,
                        frame->parse_tree
                    ),
                    &delegate_do_nothing
                );

                /* merge the parse trees */
                if(record_tree) {
                    tree_add_branch(
                        frame->caller->parse_tree,
                        frame->parse_tree
                    );
                }

                /* pop the frame */
                P_frame_stack_pop(&unused, &frame);

                /* tell the caller (now: frame) to advance its current rule list
                 * to the next rewrite rule.
                 */
                frame->curr_rule_list = (PGenericList *) list_get_next(
                    frame->curr_rule_list
                );
            }

        } else if(is_not_null(curr)) {

            curr_token = curr->token;
            curr_rule = gen_list_get_elm(frame->curr_rule_list);

            /* the next non/terminal in the current rule list is a non-terminal,
             * i.e. it is a production. we need to push the production onto the
             * stack.
             */
            if(is_not_null(curr_rule->func)) {

                /* check if we have a cached this result of applying this
                 * particular production to our current position in the token
                 * list. */
                thunk.production = curr_rule->func;
                thunk.list = curr;
                cached_result = dict_get(thunk_table, &thunk);

                /* we have a cached result. */
                if(is_not_null(cached_result)) {

                    ++num_cache_uses;

                    /* the cached result is a failure. time to backtrack. */
                    if(cached_result == P_PRODUCTION_FAILED) {
                        PARSER_DEBUG(printf("cached production failed.\n");)

                        frame->do_backtrack = 1;

                    /* the cached result was a success, we need to merge the
                     * cached tree with our current parse tree, advance to the
                     * next rewrite rule in our current rule list, and advance
                     * our current position in the token list to the token after
                     * the last token that was matched in the cached result.
                     */
                    } else {

                        PARSER_DEBUG(printf("cached production succeeded.\n");)

                        /* merge the trees */
                        if(record_tree) {
                            tree_add_branch(
                                frame->parse_tree,
                                cached_result->tree
                            );
                        }

                        /* advance to the next rewrite rule */
                        frame->curr_rule_list = (PGenericList *) list_get_next(
                            frame->curr_rule_list
                        );

                        /* advance to a future token. */
                        curr = cached_result->end;
                    }

                /* we do not have a cached result and so we will need to push a
                 * new frame onto the stack.
                 */
                } else {

                    PARSER_DEBUG(printf("pushing production onto stack.\n");)

                    ++num_func_calls;

                    P_frame_stack_push(&frame, P_frame_stack_alloc(
                        &unused,
                        prod_dict_get(P->productions, curr_rule->func),
                        curr,
                        all_parse_trees,
                        record_tree
                    ));
                }

            /* the next non/terminal in the current rule list is a terminal,
             * i.e. it is a token. we need to try to match the current token's
             * (curr_token) lexeme against the rule's lexeme. if they match then
             * we need to advance to the next rule and to the next token. if
             * they do not match then we need to signal the frame to backtrack.
             */
            } else if(P_LEXEME_EPSILON != curr_rule->lexeme) {

                ++num_tok_comparisons;

                /* record the farthest point we've gotten to. */
                if((curr_token->line > fpr.line) ||
                   (curr_token->line == fpr.line && curr_token->column > fpr.column)) {

                    fpr.column = curr_token->column;
                    fpr.line = curr_token->line;
                }

                /* we have matched a token, advance to the next token in the
                 * list and the next rewrite rule in the current rule list.
                 * also, store the matched token into the frame's partial parse
                 * tree. */
                if(curr_token->lexeme == curr_rule->lexeme) {

                    PARSER_DEBUG(if(is_not_null(curr_token->val)) {
                        printf("\t matched: %s\n", curr_token->val->str);
                    } else {
                        printf("\t matched\n");
                    })

                    /* store the match as a parse tree */
                    if(record_tree) {
                        tree_add_branch(frame->parse_tree, curr);
                    }

                    /* advance the token and rewrite rule */
                    curr = curr->next;
                    frame->curr_rule_list = (PGenericList *) list_get_next(
                        frame->curr_rule_list
                    );

                /* the tokens do not match, backtrack. */
                } else {

                    PARSER_DEBUG(printf(
                        "didn't match, expected:%d got:%d\n",
                        curr_rule->lexeme, curr_token->lexeme
                    );)

                    j++;

                    frame->do_backtrack = 1;
                }

            /* the next non/terminal in the current rule list is an empty
             * terminal, i.e. it is the empty string. we accept it, move to the
             * next rule in the current rule list, but *don't* move to the next
             * token. */
            } else {
                j++;
                frame->curr_rule_list = (PGenericList *) list_get_next(
                    frame->curr_rule_list
                );
            }

        /* the parser stack has something on it but we've reached the end of
         * input, there are two cases to deal with:
         *   i) we need to match a series of epsilon transitions, which might
         *      lead to a successful parse.
         *   ii) we need to backtrack because what we're parsing is either
         *       missing information or the current derivation is such that
         *       we expect too much information. */
        } else {

            if(is_not_null(frame->curr_rule_list)) {
                curr_rule = gen_list_get_elm(frame->curr_rule_list);
                if(P_LEXEME_EPSILON == curr_rule->lexeme) {
                    frame->curr_rule_list = (PGenericList *) list_get_next(
                        frame->curr_rule_list
                    );
                    continue;
                }
            }

            frame->do_backtrack = 1;
        }
    }

    j++; /* get gcc to not do an infinite loop. WTF */

    PARSER_DEBUG(printf(
        "\ncompleted parse with:\n\t %d token comparisons\n\t %d recursive function "
        "calls\n\t %d cache uses.\n\t %d backtracks\n\t %d cached succeses\n\t %d cached "
        "failures\n\t %d tokens\n\n",
        num_tok_comparisons, num_func_calls, num_cache_uses, num_backtracks,
        num_cached_successes, num_cached_failures, token_result.num_tokens
    );)

    /* a parse error occurred. report where we think it happened.
     *
     * TODO Improve this.
     */
    if(is_not_null(curr)) {

        j++; /* gcc ugh */
        printf(
            "A parse error occurred near line %d, column %d.\n",
            fpr.line, fpr.column
        );

    } else {

        printf("Successfully parsed.\n");

        /* transform the parse tree into an abstract syntax tree */
        gen = tree_generator_alloc(parse_tree, TREE_TRAVERSE_POSTORDER );
        while(generator_next(gen)) {
            parse_tree = generator_current(gen);
            if(P_PARSE_TREE_PRODUCTION == parse_tree->type) {
                ((PProductionTree *) parse_tree)->production(
                    (PProductionTree *) parse_tree,
                    all_parse_trees
                );
            }
        }
        generator_free(gen);
    }

    return;
}
