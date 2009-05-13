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

#define P_PRODUCTION_FAILED ((void *)(((int) NULL)+1))
#define P_SIZE_OF_REWRITE_RULE (sizeof(PParserRewriteRule) / sizeof(char))
#define P_SIZE_OF_PARSER_FUNC (sizeof(PParserFunc) / sizeof(char))

/* Type representing the necessary information to uniquely identify the result
 * of a production after being applied to a particular suffix of tokens. In this
 * case, list is some pointer to a generic list of tokens that the production was
 * applied to. The token list is ordered and never changes order and so the
 * result of a production to some part in the token list can be meaningfully
 * cached.
 */
typedef struct P_Thunk {
    PParserFunc production;
    PGenericList *list;
} P_Thunk;

/**
 * Type representing the cached result of the application of a production to
 * some part in the token list 'start'. This type is only used on successful
 * parse, the P_PRODUCTION_FAILED pointer is instead cached to indicate a failed
 * application of a production to some part of the token list.
 *
 * 'next' points to somewhere in the token list for the parser to pick up after
 * accepting the cached parse tree, 'tree', from the application of this production.
 *
 * 'start' and 'production' indicate what production was used and to what part
 * of the token list. This information is required in the event of resizing the
 * thunk hash table.
 */
typedef struct P_CachedResult {
    PGenericList *start,
                 *next;
    PParserFunc production;
    PParseTree *tree;
} P_CachedResult;

/**
 * Type representing a single production and all of its rules 'alternatives'
 * from a top-down parsing grammar. The list is of rules is explicitly ordered.
 *
 * The 'max_rule_elms' is maximum number of non/terminals in all of its rules.
 * This is used to allocate one and only one parse tree with max_rule_elms
 * branch pointers.
 */
typedef struct P_Production {
    PGenericList *alternatives;
    PParserFunc production;
    short max_rule_elms;
} P_Production;

/**
 * Type holding a pointer to the ordered list of *all* tokens from whatever text
 * is currently being parsed, as well as the number of tokens in that list.
 */
typedef struct P_TokenList {
    PGenericList *list;
    uint32_t num_tokens;
} P_TokenList;

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
 *    e) a do_backtrack flag indicating that a non/terminal in curr_rule_list
 *       has failed to match a token and so we must backtrack to the
 *       backtrack_point and try another rule, or upon full failure of the
 *       production, cascade the failure to the calling production.
 *    f) the parse tree as it is being constructed.
 *    g) the calling stack frame. This is used for two reasons, it lets us
 *       manually manage the call stack without the overhead of the generic
 *       stack data structure and it also lets us reclaim stack frames for
 *       future use.
 */
typedef struct P_StackFrame {
    P_Production *production;
    PGenericList *curr_rule_list,
                 *alternative_rules,
                 *backtrack_point;
    char do_backtrack;
    PParseTree *parse_tree;
    struct P_StackFrame *caller;
} P_StackFrame;

/**
 * Hash function that converts a thunk to a char array. Thunks are used as the
 * keys into a hash table for cached parse results.
 */
static uint32_t P_key_hash_thunk_fnc(void *pointer) { $H
    union {
        P_Thunk thunk;
        char thunk_as_chars[P_SIZE_OF_THUNK];
    } switcher;
    switcher.thunk = *((P_Thunk *) pointer);
    return_with murmur_hash(switcher.thunk_as_chars, P_SIZE_OF_THUNK, 73);
}

/**
 * Hash function to extract a thunk from a cached parse result and then hash it.
 */
static uint32_t P_val_hash_thunk_fnc(void *pointer) { $H
    P_CachedResult *res = pointer;

    P_Thunk thunk;
    thunk.list = res->start;
    thunk.production = res->production;

    return_with P_key_hash_thunk_fnc(&thunk);
}

/* Hash function that converts a token to a char array */
static uint32_t P_hash_rewrite_rule_fnc(void *rewrite_fnc) { $H
    union {
        PParserRewriteRule rewrite;
        char rule_as_chars[P_SIZE_OF_REWRITE_RULE];
    } switcher;
    switcher.rewrite = *((PParserRewriteRule *) rewrite_fnc);
    return_with murmur_hash(switcher.rule_as_chars, P_SIZE_OF_REWRITE_RULE, 73);
}

/* Hash function that converts a parser function pointer into a char array. */
static uint32_t P_key_hash_production_fnc(void *production) { $H
    union {
        PParserFunc prod;
        char prod_as_chars[P_SIZE_OF_PARSER_FUNC];
    } switcher;
    switcher.prod = production;

    return_with murmur_hash(switcher.prod_as_chars, P_SIZE_OF_REWRITE_RULE, 73);
}
static uint32_t P_val_hash_production_fnc(void *production) { $H
    return_with P_key_hash_production_fnc(((P_Production *) production)->production);
}

/**
 * Allocate a new parser on the heap. A parser, in this case, is a container
 * linking to all of top-down-parsing language data structures as well as to
 * helping deal with garbage.
 *
 * The only parameter to this function is thee production to start parsing
 * with.
 */
PParser *parser_alloc(PParserFunc start_production) { $H
    PParser *P = mem_alloc(sizeof(PParser));

    if(NULL == P) {
        mem_error("Unable to allocate a new parser on the heap.");
    }

    /* hash table mapping production function (PParserFunc) to the information
     * that corresponds with them (P_Production). */
    P->productions = dict_alloc(
        10,
        &P_key_hash_production_fnc,
        &P_val_hash_production_fnc
    );

    /* hash table mapping parser rewrite rules to themselves. This is used
     * in order to not repeatedly heap allocate the same rule twice. A rewrite
     * rule identifies a token to match, a production to call, or nothing
     * (epsilon rule) that is a blind accept.
     */
    P->rules = dict_alloc(
        15,
        &P_hash_rewrite_rule_fnc,
        &P_hash_rewrite_rule_fnc
    );

    /* the cache table, this will be allocated when we know how many tokens we
     * are working with.
     */
    P->thunk_table = NULL;

    /* a place to put unused/temporary/garbage parse trees. */
    P->temp_parse_trees = stack_alloc(sizeof(PStack));

    /* signal that this parser is open to have productions and their rules added
     * to it.
     */
    P->is_closed = 0;

    /* the production to start parsing with. */
    P->start_production = start_production;

    return_with P;
}

/**
 * Add a production to the parser's grammar. The production's name is the name
 * (or rather the function pointer) of the parser function that handles semantic
 * actions on the parse tree. This function pointer is used to reference this
 * production in the rules.
 *
 * !!! Rules are *ordered*
 */
void parser_add_production(PParser *P,
                           PParserFunc semantic_handler_fnc, /* the production name */
                           short num_seqs, /* number of rewrite sequences */
                           PParserRuleResult arg1, ...) { /* rewrite rules */
    va_list seqs;

    $H
    assert_not_null(P);
    assert_not_null(semantic_handler_fnc);
    assert(0 < num_seqs);
    assert(0 == P->is_closed);
    assert(!dict_is_set(P->productions, semantic_handler_fnc));

    PParserRuleResult curr_seq;
    PGenericList *S = NULL,
                 *curr = NULL,
                 *tail = NULL;

    P_Production *prod = mem_alloc(sizeof(P_Production));
    if(NULL == prod) {
        mem_error("Unable to allocate new production on the heap.");
    }

    prod->production = semantic_handler_fnc;
    prod->max_rule_elms = 0;

    va_start(seqs, arg1);
    for(curr_seq = arg1; \
        num_seqs > 0; \
        --num_seqs, curr_seq = va_arg(seqs, PParserRuleResult)) {

        if(prod->max_rule_elms < curr_seq.num_elms)
                prod->max_rule_elms = curr_seq.num_elms;

        curr = gen_list_alloc();
        gen_list_set_elm(curr, curr_seq.rule);

        /* add this into the sequence */
        if(NULL != tail) {
            list_set_next(tail, curr);

        /* no tail ==> need to set the head of the list. */
        } else {
            S = curr;
        }

        tail = curr;
    }

    prod->alternatives = S;

    /* add in this production */
    dict_set(P->productions, semantic_handler_fnc, prod, &delegate_do_nothing);

    return_with;
}

/**
 * Create one of the rule sequences needed for a production. Each production
 * must have one or more rule sequence, where each rule sequence is a list of
 * PParserRewriteRule telling the parser to either match a particular token or
 * to recursively call and match a production.
 */
PParserRuleResult parser_rule_sequence(short num_rules, PParserRewriteRule *arg1, ...) { $H
    va_list rules;

    $H;
    assert(0 < num_rules);
    assert_not_null(arg1);

    PParserRuleResult result;
    PParserRewriteRule *curr_rule;
    PGenericList *S = NULL,
                 *curr = NULL,
                 *tail = NULL;

    va_start(rules, arg1);
    for(curr_rule = arg1; \
        num_rules > 0; \
        --num_rules, curr_rule = va_arg(rules, PParserRewriteRule *)) {

        curr = gen_list_alloc();
        gen_list_set_elm(curr, curr_rule);

        /* add this into the sequence */
        if(NULL != tail) {
            list_set_next(tail, curr);

        /* no tail ==> need to set the head of the list. */
        } else {
            S = curr;
        }

        tail = curr;
    }


    result.num_elms = num_rules;
    result.rule = S;

    return_with result;
}

/**
 * A generic parser rewrite rule. This encompasses all three types of things that
 * we want to match:
 *   1) production (function) rules
 *   2) token rules
 *   3) epsilon rules.
 */
static PParserRewriteRule *P_parser_rewrite_rule(PParser *P,
                                                 PLexeme tok,
                                                 PParserFunc func) {
    $H
    assert_not_null(P);

    /* make a thunk out of it to search for in the hash table */
    PParserRewriteRule rewrite_rule;

    rewrite_rule.func = func;
    rewrite_rule.lexeme = tok;

    if(dict_is_set(P->rules, &rewrite_rule)) {
        return_with ((PParserRewriteRule *) dict_get(P->rules, &rewrite_rule));
    }

    // nope, need to allocate it :(
    PParserRewriteRule *R = mem_alloc(sizeof(PParserRewriteRule));
    if(NULL == R) {
        mem_error("Unable to allocate rewrite rule on the heap.");
    }

    R->func = func;
    R->lexeme = tok;

    dict_set(P->rules, &rewrite_rule, R, &delegate_do_nothing);

    return_with R;
}

/**
 * Rewrite rule for a single production function
 */
PParserRewriteRule *parser_rewrite_function(PParser *P, PParserFunc func) { $H
    assert_not_null(P);
    assert_not_null(func);
    return_with P_parser_rewrite_rule(P, P_LEXEME_EPSILON, func);
}

/**
 * Rewrite rule for a single token
 */
PParserRewriteRule *parser_rewrite_token(PParser *P, PLexeme tok) { $H
    assert_not_null(P);
    return_with P_parser_rewrite_rule(P, tok, NULL);
}

/**
 * Rewrite rule for no token required.
 */
PParserRewriteRule *parser_rewrite_epsilon(PParser *P) { $H
    assert_not_null(P);
    return_with P_parser_rewrite_rule(P, P_LEXEME_EPSILON, NULL);
}

/**
 * Allocate a new cached result of parsing on the heap.
 */
static P_CachedResult *P_alloc_cache(PGenericList *start,
                                     PGenericList *end,
                                     PParserFunc production,
                                     PParseTree *tree) { $H

    P_CachedResult *R = mem_alloc(sizeof(P_CachedResult));
    if(NULL == R) {
        mem_error("Unable to cache the result of a production on the heap.");
    }

    R->start = start;
    R->next = (PGenericList *) list_get_next(end);
    R->production = production;
    R->tree = tree;

    return_with R;
}

/**
 * Allocate a new production stack frame on the heap or re-use a previously
 * allocated but now unused one.
 */
static P_StackFrame *P_frame_stack_alloc(P_StackFrame **unused,
                                         P_Production *prod,
                                         PGenericList *backtrack_point) { $H

    P_StackFrame *frame;

    if(is_null(*unused)) {
        frame = mem_alloc(sizeof(P_StackFrame));
        if(NULL == frame) {
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

    /* make the parse tree */
    frame->parse_tree = tree_alloc(
        sizeof(PParseTree),
        (unsigned short) ((frame->production)->max_rule_elms)
    );
    frame->parse_tree->rule = 1;

    ((PParseTree *) frame->parse_tree)->production = prod->production;

    return_with frame;
}

/**
 * Push a stack frame onto the frame stack.
 */
static void P_frame_stack_push(P_StackFrame **stack, P_StackFrame *frame) { $H
    frame->caller = (*stack);
    (*stack) = frame;
    return_with;
}

/**
 * Pop a stack frame off of the frame stack and put it into the unused stack
 * where it will still be accessible.
 */
static void P_frame_stack_pop(P_StackFrame **unused, P_StackFrame **stack) { $H
    P_StackFrame *frame = (*stack);

    (*stack) = frame->caller;
    frame->caller = (*unused);
    (*unused) = frame;
    return_with;
}

/**
 * Return a linked list of all tokens in the current file being parsed.
 */
static P_TokenList P_get_all_tokens(PTokenGenerator *G) { $H
    assert_not_null(G);

    PGenericList *prev = NULL,
                 *curr = NULL;

    P_TokenList list;

    list.list = NULL;
    list.num_tokens = 0;

    /* build up a list of all of the tokens. */
    if(!generator_next(G)) {
        return_with list;
    }

    /* generate the first token */
    list.list = gen_list_alloc();
    gen_list_set_elm(list.list, generator_current(G));
    prev = list.list;
    list.num_tokens = 1;

    /* generate the rest of the tokens */
    while(generator_next(G)) {
        ++(list.num_tokens);
        curr = gen_list_alloc();
        gen_list_set_elm(curr, generator_current(G));
        list_set_next(prev, curr);
        prev = curr;
    }
    return_with list;
}

/**
 * Parse the tokens from a token generator. This parser is implemented in a
 * very similar way to packrat parsing, found here:
 *
 *       http://pdos.csail.mit.edu/~baford/packrat/thesis/thesis.pdf
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
 * This algorithm runs in O(n) time, where n is the number of tokens to parse.
 */
static PParseTree *P_parse_tokens(PParser *P, \
                            PTokenGenerator *G) { $H

    assert_not_null(P);
    assert_not_null(G);
    assert(dict_is_set(P->productions, P->start_production));

    /* close the parser to updates on its grammar. */
    P->is_closed = 1;

    PToken *curr_token = NULL;
    P_Production *curr_production = NULL;
    P_Thunk thunk;
    P_TokenList token_result = P_get_all_tokens(G);
    PParseTree *parse_tree = NULL;
    PParserRewriteRule *curr_rule;
    P_StackFrame *frame = NULL,
                 *unused = NULL;
    PGenericList *curr = NULL;;

    /* no tokens were lexed,
     * TODO: do something slightly more useful. */
    if(0 == token_result.num_tokens) {
        return_with parse_tree;
    }

    curr = token_result.list;

    /* allocate enough space for a single perfect pass through the text for
     * thunks.
     */
    P->thunk_table = dict_alloc(
        token_result.num_tokens,
        &P_key_hash_thunk_fnc,
        &P_val_hash_thunk_fnc
    );

    /* get the starting production and push our first stack frame on. This
     * involves registering the start of the token list as the furthest back
     * we can backtrack.
     */
    curr_production = dict_get(P->productions, P->start_production);
    P_frame_stack_push(
        &frame,
        P_frame_stack_alloc(&unused, curr_production, curr)
    );

    while(!is_null(frame) && !is_null(curr)) {

        /*frame = stack_peek(parser_stack);*/
        curr_production = frame->production;

        /* the production on the top of the stack has been signaled to
         * backtrack. */
        if(frame->do_backtrack) {

            /* this production has failed by virtue of having no more rules
             * to try. we need to record this failure, pop off the stack, and
             * cascade our failure upward. */
            if(is_null(frame->alternative_rules)) {

                /* record the thunk for future reference */
                thunk.production = curr_production->production;
                thunk.list = frame->backtrack_point;
                dict_set(
                   P->thunk_table, &thunk,
                   P_PRODUCTION_FAILED, &delegate_do_nothing
                );

                /* trim off the branches of this frame's parse tree, then free
                 * the tree. We do a trim because productions called in/directly
                 * through this production might have succeeded and hence might
                 * be future useful results. */
                tree_trim(frame->parse_tree, P->temp_parse_trees);
                tree_free(frame->parse_tree, &delegate_do_nothing);
                frame->parse_tree = NULL;

                /* pop off this frame and continue on. */
                P_frame_stack_pop(&unused, &frame);

                /* cascade this failure to the next stack frame. */
                if(!is_null(frame)) {
                    frame->do_backtrack = 1;
                }

                continue;
            }

            /* try the next rule in this production */
            frame->curr_rule_list = (PGenericList *) gen_list_get_elm(
                frame->alternative_rules
            );

            frame->alternative_rules = (PGenericList *) list_get_next(
                frame->alternative_rules
            );

            frame->do_backtrack = 0;
            ++(frame->parse_tree->rule);

            /* clear the parse tree and store the unused trees elsewhere */
            tree_trim(frame->parse_tree, P->temp_parse_trees);

        /* the production on the top of the stack succeeded. merge it into the
         * parent tree or return the parse tree. Also, cache the production as
         * a thunk. */
        } else if(is_null(frame->curr_rule_list)) {

            P_frame_stack_pop(&unused, &frame);

            /* done parsing, yay! */
            if(!is_null(frame)) {
                parse_tree = unused->parse_tree;
                goto done_parsing;
            }

            thunk.list = unused->backtrack_point;
            thunk.production = curr_production->production;

            /* cache the result of this production */
            dict_set(
                P->thunk_table,
                &thunk,
                P_alloc_cache(
                    unused->backtrack_point,
                    curr,
                    curr_production->production,
                    unused->parse_tree
                ),
                &delegate_do_nothing
            );

            /* add the parse tree to the parent tree's derivation */
            tree_add_branch(
                frame->parse_tree,
                unused->parse_tree
            );

            continue;
        }

        /* get the rewrite rule and the current token we are looking at. */
        curr_rule = gen_list_get_elm(frame->curr_rule_list);
        curr_token = gen_list_get_elm(curr);

        /* if the current rewrite rule's function pointer is not null then it
         * is a recursive call to another production and we must push that
         * production onto the stack.
         */
        if(!is_null(curr_rule->func)) {

            /* check if we have a memoized this result. */
            thunk.list = curr;
            thunk.production = curr_rule->func;

            /* yes we have! take the resulting parse tree, add it in to our
             * frame and then advance to after all of the parsed tokens. */
            if(dict_is_set(P->thunk_table, &thunk)) {

                P_CachedResult *result = dict_get(P->thunk_table, &thunk);
                tree_add_branch(frame->parse_tree, result->tree);
                curr = result->next;

            /* nope, we need to perform the parse. */
            } else {
                P_frame_stack_push(&frame, P_frame_stack_alloc(
                    &unused,
                    dict_get(P->productions, curr_rule->func),
                    curr
                ));
            }

            continue;

        /* we need to try to match the current token against the token expected
         * in this production's current rule.
         */
        } else if(P_LEXEME_EPSILON != curr_rule->lexeme) {

            /* ugh. we failed to match and so we must backtrack. */
            if(curr_rule->lexeme != curr_token->lexeme) {
                frame->do_backtrack = 1;
                continue;
            }

            /* we have matched the token. */
            tree_add_branch(frame->parse_tree, curr_token);

            /* advance things :) */
            curr = (PGenericList *) list_get_next(curr);
            frame->curr_rule_list = (PGenericList *) list_get_next(
                frame->curr_rule_list
            );

        /* we have found an epsilon token, i.e. an empty character. we accept it
         * automatically and advance to the next rule.
         */
        } else {
            frame->curr_rule_list = (PGenericList *) list_get_next(
                frame->curr_rule_list
            );

            /* TODO: record? */
        }
    }

    /* we've run out of tokens but there is still stuff to parse, i.e. parse
     * error.
     *
     * TODO
     */
    if(!is_null(frame)) {
        printf("parse error, not enough tokens!\n");
        exit(1);
    }

    /* TODO */
    done_parsing:
    printf("parsed successfully!\n");

    return_with parse_tree;
}

