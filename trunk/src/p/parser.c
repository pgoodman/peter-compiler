/*
 * p-parser.c
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-parser.h>

#define P_PRODUCTION_TRY_LR ((void *) 11)
#define PARSER_DEBUG(x) x

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
PParseTree *parser_parse_tokens(PParser *P, PTokenGenerator *G) {

    PToken *curr_token = NULL;
    P_TerminalTreeList token_result;
    PParseTree *parse_tree = NULL;
    PParserRewriteRule *curr_rule;

    P_ProductionStack *production_stack = NULL;

    P_ProductionCache *parser_cache = NULL;
    P_ProductionCacheValue *cached_result = NULL;

    PTerminalTree *curr = NULL;
    PDictionary *all_parse_trees = NULL;
    PTreeGenerator *gen = NULL;

    /* stats */
    int num_tok_comparisons = 0,
        num_func_calls = 0,
        num_cache_uses = 0,
        num_backtracks = 0,
        num_cached_successes = 0,
        num_cached_failures = 0;

    /* useful counter, be it for hinting to GCC that a block shouldn't be
     * optimized out, or for actually counting something. */
    unsigned long int j = 0;

    /* for parse error info, the farthest point reached (fpr) in the token
     * stream. */
    struct {
        uint32_t line;
        uint32_t column;
    } fpr;

    assert_not_null(P);
    assert_not_null(G);
    assert(is_not_null(P->productions + P->start_production));

    /* error reporting info */
    fpr.column = 0;
    fpr.line = 0;

    /* set for all trees to go in. while constructing the rCST from the CST we
     * will be removing some nodes and keeping others. We use this dictionary
     * to first track *all* trees, then we will remove the trees that we are
     * _keeping_ in the rCST from the dictionary so that all the remaining trees
     * in the set can be freed.
     */
    all_parse_trees = P_tree_set_alloc();
    token_result = P_tree_alloc_terminals(all_parse_trees, G);

    /* close the parser to updates on its grammar. */
    P->is_closed = 1;

    /* no tokens were lexed, TODO: do something slightly more useful. */
    if(0 == token_result.num_tokens) {
        return NULL;
    }

    curr = token_result.list;

    /* allocate enough space for a single perfect pass through the text for
     * thunks.
     */
    parser_cache = P_cache_alloc(token_result.num_tokens);

    production_stack = P_production_alloc();


    /* get the starting production and push our first stack frame on. This
     * involves registering the start of the token list as the farthest back
     * we can backtrack.
     */
    PARSER_DEBUG(printf("pushing production onto stack.\n");)
    ++num_func_calls;
    P_production_push(
        production_stack,
        parser_cache,
        all_parse_trees,
        P->productions + P->start_production,
        curr
    );

    while(P_production_can_cascade(production_stack)) {

        /* an error occurred in the current frame. we need to pop it off, dump
         * its parse tree, backtrack, and possibly cascade the failure upward.
         */
        if(P_production_rule_failed(production_stack)) {
            ++num_backtracks;

            /* there are no rules to backtrack to, we need to cascade the
             * failure upward, dump the tree, and cache the failure.
             */
            if(!P_production_can_backtrack(production_stack)) {

                PARSER_DEBUG(printf("cascading.\n");)

                ++num_cached_failures;

                P_production_failed(production_stack, parser_cache);

                /* check if we encountered a parse error. */
                if(!P_production_can_cascade(production_stack)) {
                    PARSER_DEBUG(printf("parse error.\n");)
                    break;
                }

                /* cascade the failure to the calling production. */
                P_production_cascade(production_stack, parser_cache);

            /* there is at least one rule to backtrack to. */
            } else {
                PARSER_DEBUG(printf("backtracking.\n");)
                curr = P_production_backtrack(production_stack);
            }

        /* a production successfully matched all of the non/terminals in its
         * current rule list. we need to pop it off, merge its parse tree into
         * the parent frame's (calling production) parse tree, and then tell
         * the parent frame to advance itself to the next non/terminal in its
         * current rule list. if there is no parent frame then we have
         * successfully parsed the tokens.
         */
        } else if(!P_production_has_rule(production_stack)) {

            /* we have parsed all of the tokens. there is no need to do any
             * work on the stack or the cache. */
            if(P_production_is_root(production_stack) && is_null(curr)) {
                PARSER_DEBUG(printf("done parsing.\n");)

                j++; /* make GCC not optimize out this block. */
                break;

            } else {

                cached_result = P_production_get_cache(
                    production_stack,
                    parser_cache
                );

                /* we had a left recursive application, let's restart it to grow the left
                 * recursive derivation further. */
                if(P_cache_value_is_left_recursive(cached_result)) {

                    PARSER_DEBUG(printf("growing left recursion. \n");)

                    P_production_grow_lr(
                        production_stack,
                        parser_cache,
                        cached_result,
                        & curr
                    );

                /* there are tokens to parse but we have a single frame on
                 * on stack. this is a parse error, so we will backtrack. */
                } else if(P_production_is_root(production_stack) && is_not_null(curr)) {

                    PARSER_DEBUG(printf("no rules left. \n");)
                    P_production_break(production_stack);

                /* we can't prove that this is a successful parse of all of the
                 * tokens and so we assume that there exists more to parse. save
                 * the result of the application of this production to the cache
                 * and yield control to the parent frame.
                 */
                } else {
                    PARSER_DEBUG(printf("production succeeded.\n");)

                    ++num_cached_successes;

                    /* pop the production and advance to the next rule of the
                     * calling production. */
                    P_production_succeeded(production_stack, parser_cache, curr);
                }
            }

        } else if(is_not_null(curr)) {

            curr_token = curr->token;
            curr_rule = P_production_get_rule(production_stack);

            /* the next non/terminal in the current rule list is a non-terminal,
             * i.e. it is a production. we need to push the production onto the
             * stack.
             */
            if(P_adt_rule_is_production(curr_rule)) {

match_production:

                /* check if we have a cached this result of applying this
                 * particular production to our current position in the token
                 * list. */
                cached_result = P_cache_get(
                    parser_cache,
                    curr_rule->production,
                    curr
                );

                /* we have a cached result. */
                if(is_not_null(cached_result)) {

                    ++num_cache_uses;

                    /* the cached result is a failure. time to backtrack. */
                    if(P_cache_value_is_failure(cached_result)) {

                        PARSER_DEBUG(printf("cached production failed.\n");)
                        P_production_break(production_stack);

                    /* the cached result is missing, i.e. the cache value was
                     * initialized by a production already on the stack but that
                     * has (in)directly called itself through left recursion. */
                    } else if(P_cache_value_is_missing(cached_result)) {

                        PARSER_DEBUG(printf("detected left recursion. \n");)

                        ++num_func_calls;

                        P_cache_value_enable_left_recursion(cached_result);
                        P_production_push_lr(
                            production_stack,
                            parser_cache,
                            all_parse_trees,
                            P->productions + (curr_rule->production),
                            curr
                        );

                    /* the cached result was a success, we need to merge the
                     * cached tree with our current parse tree, advance to the
                     * next rewrite rule in our current rule list, and advance
                     * our current position in the token list to the token after
                     * the last token that was matched in the cached result.
                     */
                    } else {

                        PARSER_DEBUG(printf("cached production succeeded.\n");)

                        P_production_cache_succeeded(
                            production_stack,
                            cached_result
                        );

                        /* advance to a future token. */
                        curr = P_cache_value_get_terminal(cached_result);
                    }

                /* we do not have a cached result and so we will need to push a
                 * new frame onto the stack.
                 */
                } else {
                    PARSER_DEBUG(printf("pushing production onto stack.\n");)

                    ++num_func_calls;

                    P_production_push(
                        production_stack,
                        parser_cache,
                        all_parse_trees,
                        P->productions + (curr_rule->production),
                        curr
                    );
                }

            /* the next non/terminal in the current rule list is a terminal,
             * i.e. it is a token. we need to try to match the current token's
             * (curr_token) lexeme against the rule's lexeme. if they match then
             * we need to advance to the next rule and to the next token. if
             * they do not match then we need to signal the frame to backtrack.
             */
            } else if(P_adt_rule_is_token(curr_rule)) {

                ++num_tok_comparisons;

                /* record the farthest point we've gotten to. */
                if(P_tree_progress_was_made(curr, fpr.line, fpr.column)) {
                    fpr.column = curr_token->column;
                    fpr.line = curr_token->line;
                }

                /* we have matched a token, advance to the next token in the
                 * list and the next rewrite rule in the current rule list.
                 * also, store the matched token into the frame's partial parse
                 * tree. */
                if(curr_token->token == curr_rule->token) {

                    PARSER_DEBUG(if(is_not_null(curr_token->lexeme)) {
                        printf("\t matched: %s\n", curr_token->lexeme->str);
                    } else {
                        printf("\t matched \n");
                    })

                    /* store the match as a parse tree */
                    if(P_adt_rule_is_non_excludable(curr_rule)) {
                        tree_force_add_branch(
                            (PTree *) P_production_get_parse_tree(
                                production_stack
                            ),
                            (PTree *) curr
                        );
                    }

                    /* advance the token and rewrite rule */
                    curr = curr->next;
                    P_production_continue(production_stack);

                /* the tokens do not match, backtrack. */
                } else {

                    PARSER_DEBUG(printf(
                        "\t didn't match, expected:%d got:%d\n",
                        curr_rule->token, curr_token->token
                    );)

                    P_production_break(production_stack);
                }

            /* the next non/terminal in the current rule list is an empty
             * terminal, i.e. it is the empty string. we accept it, move to the
             * next rule in the current rule list, but *don't* move to the next
             * token. */
            } else {
match_epsilon:

                PARSER_DEBUG(printf("\t matched epsilon. \n");)

                if(P_adt_rule_is_non_excludable(curr_rule)) {
                    parse_tree = P_tree_alloc_epsilon();
                    P_tree_set_add(all_parse_trees, parse_tree);

                    tree_force_add_branch(
                        (PTree *) P_production_get_parse_tree(production_stack),
                        (PTree *) parse_tree
                    );
                }

                P_production_continue(production_stack);
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
        } else {

            PARSER_DEBUG(printf("end of input. \n");)

            if(P_production_has_rule(production_stack)) {
                curr_rule = P_production_get_rule(production_stack);

                if(P_adt_rule_is_epsilon(curr_rule)) {
                    goto match_epsilon;
                } else if(P_adt_rule_is_production(curr_rule)) {
                    goto match_production;
                } else {
                    P_production_break(production_stack);
                }
            } else {
                P_production_break(production_stack);
            }
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

        parse_tree = NULL;

    } else {

        printf("Successfully parsed.\n");
        printf("Num trees in garbage: %d \n", all_parse_trees->num_used_slots);

        parse_tree = P_production_get_parse_tree(production_stack);

        printf("removing valid trees from garbage... \n");

        /* go over our reduced parse tree and remove any references to trees
         * listed in our all trees hash table to that we can free all of the
         * remaining trees in there at once. */
        gen = tree_generator_alloc((PTree *) parse_tree, TREE_TRAVERSE_POSTORDER);
        j = 0;
        while(generator_next(gen)) {
            P_tree_set_remove(all_parse_trees, generator_current(gen));
            ++j;
        }

        generator_free(gen);

        printf("Num nodes in tree: %ld \n", j);
        printf("Num trees in garbage: %d \n", all_parse_trees->num_used_slots);
    }

    printf("freeing resources... \n");

    /* free out the resources no longer needed. */
    P_tree_set_free(all_parse_trees);

    printf("garbage trees freed. \n");

    P_cache_free(parser_cache);

    printf("cache freed. \n");

    P_Production_free(production_stack);

    printf("production stack freed. \n");

    printf("returning parse tree...\n");

    return parse_tree;
}
