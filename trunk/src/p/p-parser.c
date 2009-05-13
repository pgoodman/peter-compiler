/*
 * p-parser.c
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-parser.h>

#define P_SIZE_OF_THUNK (0 \
         + sizeof(PParserFunc *) \
         + sizeof(PGenericList *) \
        ) / sizeof(char)

#define P_MAX_RECURSION_DEPTH 100

#define P_PRODUCTION_FAILED ((void *)(((int) NULL)+1))

/*
#define P_SIZE_OF_REWRITE_RULE ((sizeof(PParserRewriteFunc) > sizeof(PParserRewriteToken)) \
        ? sizeof(PParserRewriteFunc) \
        : sizeof(PParserRewriteToken))

#define P_SIZE_OF_CANONICAL_REWRITE_RULE sizeof(P_CanonincalRewriteRule) / sizeof(char)
*/

#define P_SIZE_OF_REWRITE_RULE (sizeof(PParserRewriteRule) / sizeof(char))
#define P_SIZE_OF_PARSER_FUNC (sizeof(PParserFunc) / sizeof(char))

/* type describing a type used to store a lazy result for a function */
typedef struct P_Thunk {
    PParserFunc production;
    PGenericList *list;
} P_Thunk;

typedef struct P_CachedResult {
    PGenericList *start,
                 *next;
    PParserFunc production;
    PParseTree *tree;
} P_CachedResult;

/* call stack type. */

/* production info. */
typedef struct P_Production {
    PGenericList *alternatives;
    PParserFunc production;
    short max_rule_elms;
} P_Production;

typedef struct P_TokenList {
    PGenericList *list;
    uint32_t num_tokens;
} P_TokenList;

typedef struct P_StackFrame {
    P_Production *production;
    PGenericList *curr_rule_list,
                 *alternative_rules,
                 *backtrack_point;
    char do_backtrack;
    PParseTree *parse_tree;
} P_StackFrame;

/* Hash function that converts a thunk to a char array */
static uint32_t P_key_hash_thunk_fnc(void *pointer) { $H
    union {
        P_Thunk thunk;
        char thunk_as_chars[P_SIZE_OF_THUNK];
    } switcher;
    switcher.thunk = *((P_Thunk *) pointer);
    return_with murmur_hash(switcher.thunk_as_chars, P_SIZE_OF_THUNK, 73);
}

/* Hash what the cache for thunks points to back into the hash value. */
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
 * Allocate a new parser on the heap.
 */
PParser *parser_alloc(void) { $H
    PParser *P = mem_alloc(sizeof(PParser));

    if(NULL == P) {
        mem_error("Unable to allocate a new parser on the heap.");
    }

    P->productions = dict_alloc(10, &P_key_hash_production_fnc, &P_val_hash_production_fnc);
    P->rules = dict_alloc(15, &P_hash_rewrite_rule_fnc, &P_hash_rewrite_rule_fnc);
    P->thunk_table = NULL;
    P->temp_parse_trees = stack_alloc(sizeof(PStack));

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
 * Create one of the rule sequences needed for a production.
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
 * A generic parser rewrite rule. This encompasses all three types:
 * 1) production (function) rules
 * 2) token rules
 * 3) epsilon rules.
 */
static PParserRewriteRule *P_parser_rewrite_rule(PParser *P, PLexeme tok, PParserFunc func) { $H
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

/* Rewrite rule for a single function */
PParserRewriteRule *parser_rewrite_function(PParser *P, PParserFunc func) { $H
    assert_not_null(P);
    assert_not_null(func);
    return_with P_parser_rewrite_rule(P, P_LEXEME_EPSILON, func);
}

/* Rewrite rule for a single token */
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

static P_StackFrame *P_alloc_stack_frame(P_Production *prod,
                                         PGenericList *backtrack_point) { $H

    P_StackFrame *frame = mem_alloc(sizeof(P_StackFrame));
    if(NULL == frame) {
        mem_error("Unable to allocate new parser stack frame on the heap.");
    }

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

    return frame;
}

/* Return a linked list of all tokens in the current file being parsed */
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
 * Parse the tokens from a token generator.
 */
static PParseTree *P_parse_tokens(PParser *P, \
                            PTokenGenerator *G, \
                            PParserFunc start_production) { $H

    assert_not_null(P);
    assert_not_null(G);
    assert(dict_is_set(P->productions, start_production));

    /* close the parser to updates on its grammar. */
    P->is_closed = 1;

    PStack *parser_stack = stack_alloc(sizeof(PStack));
    PToken *curr_token = NULL;
    P_Production *curr_production = NULL;
    P_Thunk thunk;
    P_TokenList token_result = P_get_all_tokens(G);
    PParseTree *parse_tree = NULL;

    /* no tokens were lexed */
    if(0 == token_result.num_tokens) {
        return_with parse_tree;
    }

    /* list of tokens available for backtracking. */
    PGenericList *token_list = token_result.list,

                 /* the list with the token in it */
                 *curr = token_list;

    PParserRewriteRule *curr_rule;
    P_StackFrame *frame = NULL;

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
    curr_production = dict_get(P->productions, start_production);
    stack_push(parser_stack, P_alloc_stack_frame(curr_production, token_list));


    while(!stack_is_empty(parser_stack) && !is_null(curr)) {

        frame = stack_peek(parser_stack);

        /* the production on the top of the stack has been signalled to
         * backtrack. */
        if(frame->do_backtrack) {

            /* this production has failed by virtue of having no more rules
             * to try. we need to record this failure, pop off the stack, and
             * cascade our failure upward. */
            if(NULL == frame->alternative_rules) {

                /* record the thunk for future reference */
                thunk.production = (frame->production)->production;
                thunk.list = frame->backtrack_point;
                dict_set(
                   P->thunk_table, &thunk,
                   P_PRODUCTION_FAILED, &delegate_do_nothing
                );

                /* pop off this frame and continue on. */
                stack_pop(parser_stack);

                /* free this frame's parse tree */
                tree_free(frame->parse_tree, &delegate_do_nothing);

                /* free the frame. */
                mem_free(frame);

                /* cascade this failure to the next stack frame. */
                if(!stack_is_empty(parser_stack)) {
                    ((P_StackFrame *) stack_peek(parser_stack))->do_backtrack = 1;
                }

                continue;
            }

            /* try the next rule in this production */
            frame->curr_rule_list = (PGenericList *) gen_list_get_elm(frame->alternative_rules);
            frame->alternative_rules = (PGenericList *) list_get_next(frame->alternative_rules);
            frame->do_backtrack = 0;
            ++(frame->parse_tree->rule);

            /* clear the parse tree and store the unused trees elsewhere */
            tree_trim(frame->parse_tree, P->temp_parse_trees);

        /* the production on the top of the stack succeeded. merge it into the
         * parent tree or return the parse tree. Also, cache the production as
         * a thunk. */
        } else if(is_null(frame->curr_rule_list) || !list_has_next(frame->curr_rule_list)) {

            stack_pop(parser_stack);

            /* done parsing, yay! */
            if(stack_is_empty(parser_stack)) {
                parse_tree = frame->parse_tree;
                mem_free(frame);
                return_with parse_tree;
            }

            thunk.list = frame->backtrack_point;
            thunk.production = (frame->production)->production;

            /* cache the result of this production */
            dict_set(
                P->thunk_table,
                &thunk,
                P_alloc_cache(
                    frame->backtrack_point,
                    curr,
                    (frame->production)->production,
                    frame->parse_tree
                ),
                &delegate_do_nothing
            );

            /* add the parse tree to the parent tree's derivation */
            tree_add_branch(
                ((P_StackFrame *) stack_peek(parser_stack))->parse_tree,
                frame->parse_tree
            );

            /* free the frame and continue. */
            mem_free(frame);
            continue;
        }

        /* get the rewrite rule and the current token we are looking at. */
        curr_rule = gen_list_get_elm(frame->curr_rule_list);
        curr_token = gen_list_get_elm(curr);

        /* if the current rewrite rule's function pointer is not null then it
         * is a recursive call to another production and we must push that
         * production onto the stack.
         */
        if(NULL != curr_rule->func) {

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
                stack_push(parser_stack, P_alloc_stack_frame(
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
            frame->curr_rule_list = (PGenericList *) list_get_next(frame->curr_rule_list);
            curr = (PGenericList *) list_get_next(curr);

        /* we have found an epsilon token, i.e. an empty character. we accept it
         * automatically and advance to the next rule.
         */
        } else {
            frame->curr_rule_list = (PGenericList *) list_get_next(frame->curr_rule_list);

            /* TODO: record? */
        }
    }

    /* we've run out of tokens but there is still stuff to parse, i.e. parse
     * error.
     *
     * TODO
     */
    if(!stack_is_empty(parser_stack)) {
        printf("parse error, not enough tokens!\n");
        exit(1);
    }

    return_with parse_tree;
}

