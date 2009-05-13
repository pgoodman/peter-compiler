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
         + sizeof(PToken *) \
         + sizeof(PParserTokenList *) \
        ) / sizeof(char)

#define P_MAX_RECURSION_DEPTH 100

/*
#define P_SIZE_OF_REWRITE_RULE ((sizeof(PParserRewriteFunc) > sizeof(PParserRewriteToken)) \
        ? sizeof(PParserRewriteFunc) \
        : sizeof(PParserRewriteToken))

#define P_SIZE_OF_CANONICAL_REWRITE_RULE sizeof(P_CanonincalRewriteRule) / sizeof(char)
*/

#define P_SIZE_OF_REWRITE_RULE (sizeof(PParserRewriteRule) / sizeof(char))
#define P_SIZE_OF_PARSER_FUNC (sizeof(PParserFunc) / sizeof(char))

/* type describing a type used to store a lazy result for a function */
typedef struct PParserThunk {
    PParserFunc *func;
    PToken *car;
    PParserTokenList *cdr;
} P_Thunk;


/* call stack type. */
typedef struct P_CallStack {
    PList _;
    short local_extent;
    PParserRewriteRule *rewrite_rule;
} P_CallStack;

/* production info. */
typedef struct P_Production {
    PGenericList *alternatives;
    PParserFunc production;
    short num_alternatives;
} P_Production;

/* Hash function that converts a thunk to a char array */
static uint32_t P_hash_thunk_fnc($$ void *pointer ) { $H
    union {
        P_Thunk thunk;
        char thunk_as_chars[P_SIZE_OF_THUNK];
    } switcher;
    switcher.thunk = *((P_Thunk *) pointer);
    return_with murmur_hash(switcher.thunk_as_chars, P_SIZE_OF_THUNK, 73);
}

/* Hash function that converts a token to a char array */
static uint32_t P_hash_rewrite_rule_fnc($$ void *rewrite_fnc) { $H
    union {
        PParserRewriteRule rewrite;
        char rule_as_chars[P_SIZE_OF_REWRITE_RULE];
    } switcher;
    switcher.rewrite = *((PParserRewriteRule *) rewrite_fnc);
    return_with murmur_hash(switcher.rule_as_chars, P_SIZE_OF_REWRITE_RULE, 73);
}

/* Hash function that converts a parser function pointer into a char array. */
static uint32_t P_key_hash_production_fnc($$ void *production) {
    union {
        PParserFunc prod;
        char prod_as_chars[P_SIZE_OF_PARSER_FUNC];
    } switcher;
    switcher.prod = production;
    return_with murmur_hash(switcher.prod_as_chars, P_SIZE_OF_REWRITE_RULE, 73);
}
static uint32_t P_val_hash_production_fnc($$ void *production) {
    return_with P_key_hash_production_fnc($$A ((P_Production *) production)->production);
}

/**
 * Allocate a new parser on the heap.
 */
PParser *parser_alloc($) { $H
    PParser *P = mem_alloc(sizeof(PParser));

    if(NULL == P) {
        mem_error("Unable to allocate a new parser on the heap.");
    }

    P->productions = dict_alloc($$A 10, &P_key_hash_production_fnc, &P_val_hash_production_fnc);
    P->rules = dict_alloc($$A 15, &P_hash_rewrite_rule_fnc, &P_hash_rewrite_rule_fnc);

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
void parser_add_production($$ PParser *P,
                           PParserFunc semantic_handler_fnc, /* the production name */
                           short num_seqs, /* number of rewrite sequences */
                           PGenericList *arg1, ...) { /* rewrite rules */
    va_list seqs;

    $H
    assert_not_null(P);
    assert_not_null(semantic_handler_fnc);
    assert(0 < num_seqs);
    assert_not_null(arg1);
    assert(0 == P->is_closed);
    assert(!dict_is_set($$A P->productions, semantic_handler_fnc));

    PGenericList *curr_seq;
    PGenericList *S = NULL,
                 *curr = NULL,
                 *tail = NULL;

    P_Production *prod = mem_alloc(sizeof(P_Production));
    if(NULL == prod) {
        mem_error("Unable to allocate new production on the heap.");
    }

    prod->num_alternatives = num_seqs;
    prod->production = semantic_handler_fnc;

    va_start(seqs, arg1);
    for(curr_seq = arg1; \
        num_seqs > 0; \
        --num_seqs, curr_seq = va_arg(seqs, PGenericList *)) {

        curr = gen_list_alloc($A);
        gen_list_set_elm($$A curr, curr_seq);

        /* add this into the sequence */
        if(NULL != tail) {
            list_set_next($$A tail, curr);

        /* no tail ==> need to set the head of the list. */
        } else {
            S = curr;
        }

        tail = curr;
    }

    prod->alternatives = S;

    /* add in this production */
    dict_set($$A P->productions, semantic_handler_fnc, prod, &delegate_do_nothing);

    return_with;
}

/**
 * Create one of the rule sequences needed for a production.
 */
PGenericList *parser_rule_sequence($$ short num_rules, PParserRewriteRule *arg1, ...) { $H
    va_list rules;

    $H;
    assert(0 < num_rules);
    assert_not_null(arg1);

    PParserRewriteRule *curr_rule;
    PGenericList *S = NULL,
                 *curr = NULL,
                 *tail = NULL;

    va_start(rules, arg1);
    for(curr_rule = arg1; \
        num_rules > 0; \
        --num_rules, curr_rule = va_arg(rules, PParserRewriteRule *)) {

        curr = gen_list_alloc($A);
        gen_list_set_elm(curr, curr_rule);

        /* add this into the sequence */
        if(NULL != tail) {
            list_set_next($$A tail, curr);

        /* no tail ==> need to set the head of the list. */
        } else {
            S = curr;
        }

        tail = curr;
    }

    return_with S;
}

/**
 * A generic parser rewrite rule. This encompases all three types:
 * 1) production (function) rules
 * 2) token rules
 * 3) epsilon rules.
 */
static PParserRewriteRule *P_parser_rewrite_tule($$ PParser *P, PLexeme tok, PParserFunc func) { $H
    assert_not_null(P);

    /* make a thunk out of it to search for in the hash table */
    PParserRewriteRule rewrite_rule;

    rewrite_rule.func = func;
    rewrite_rule.lexeme = tok;

    if(dict_is_set($$A P->rules, &rewrite_rule)) {
        return_with ((PParserRewriteRule *) dict_get($$A P->rules, &rewrite_rule));
    }

    // nope, need to allocate it :(
    PParserRewriteRule *R = mem_alloc(sizeof(PParserRewriteRule));
    if(NULL == R) {
        mem_error("Unable to allocate rewrite rule on the heap.");
    }

    R->func = func;
    R->lexeme = tok;

    dict_set($$A P->rules, &rewrite_rule, R, &delegate_do_nothing);

    return_with R;
}

/* Rewrite rule for a single function */
PParserRewriteRule *parser_rewrite_function($$ PParser *P, PParserFunc func) { $H
    assert_not_null(P);
    assert_not_null(func);
    return_with P_parser_rewrite_tule($$A P, P_LEXEME_EPSILON, func);
}

/* Rewrite rule for a single token */
PParserRewriteRule *parser_rewrite_token($$ PParser *P, PLexeme tok) { $H
    assert_not_null(P);
    return_with P_parser_rewrite_tule($$A P, tok, NULL);
}

/**
 * Rewrite rule for no token required.
 */
PParserRewriteRule *parser_rewrite_epsilon($$ PParser *P) { $H
    assert_not_null(P);
    return_with P_parser_rewrite_tule($$A P, P_LEXEME_EPSILON, NULL);
}

/* Return a linked list of all tokens in the current file being parsed */
static PGenericList *P_get_all_tokens($$ PTokenGenerator *G) { $H
    assert_not_null(G);

    PGenericList *token_list = NULL,
                 *prev = NULL,
                 *curr = NULL;

    /* build up a list of all of the tokens. */
    if(!generator_next($$A G)) {
        return_with NULL;
    }

    /* generate the first token */
    token_list = gen_list_alloc($A);
    gen_list_set_elm($$A token_list, generator_current($$A G));
    prev = token_list;

    /* generate the rest of the tokens */
    while(generator_next($$A G)) {
        curr = gen_list_alloc($A);
        gen_list_set_next(prev, curr);
        gen_list_set_elm($$A curr, generator_current(G));
        prev = curr;
    }

    return_with token_list;
}

static void P_run_parser($$ PParser *P, PTokenGenerator *G) { $H
    assert_not_null(P);
    assert_not_null(G);

    short current_return_point = -1;

    PStack *call_stack = stack_alloc($$A sizeof(PStack)),
           *backtrack_stack = stack_alloc($$A sizeof(PStack)),
           *backtrack_points = stack_alloc($$A sizeof(PStack));

    PToken *curr_token = NULL,
           *next_token = NULL;

    /* list of tokens available for backtracking. */
    PGenericList *token_list = P_get_all_tokens($$A G),

                 /* list of tokens that we no longer need for backtracking
                  * purposes */
                 *cut_list = NULL,

                 /* the list with the token in it */
                 *curr = token_list,

                 /* rewrite rule from the top of stack being used. */
                 *curr_rewrite_rule = NULL;

    if(NULL == curr) {
        return_with NULL;
    }

    do {

    } while(NULL != curr);

#if 0
    /* start everything off */
    curr = gen_list_alloc();
    active_list = curr;
    gen_list_set_elm(active_list, generator_get());

    while(NULL != curr) {

        curr_token = (PToken *) gen_list_get_elm(curr);


        /* we need to generate the next token and set it to the tail of our
         * list.
         */
        if(!list_has_next($$A curr)) {

            /* no more tokens available. */
            if(!generator_next(G)) {
                break;
            }

            curr_token = (PToken *) generator_current(G);
            temp = gen_list_alloc($A);
            gen_list_set_elm($$A temp, curr_token);
            gen_list_set_next($$A tail, curr_token);

            tail = temp;

        /* we have already lexed the next token, recover it from memory. */
        } else {
            tail = list_get_next($$A tail);
            curr_token = (PToken *) gen_list_get_elm($$A tail);
        }
    }
#endif
    return_with;
}

