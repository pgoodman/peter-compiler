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

/* An ordered sequence of rule sequences for a production. */
typedef struct P_RuleSequenceSequence {
    PList _;
    PParserRuleSequence *seq;
} P_RuleSequenceSequence;


/* call stack type. */
typedef struct P_CallStack {
    PList _;
    short local_extent;
    PParserRewriteRule *rewrite_rule;
} P_CallStack;

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
static uint32_t P_hash_production_fnc($$ void *production) {
    union {
        PParserFunc prod;
        char prod_as_chars[P_SIZE_OF_PARSER_FUNC];
    } switcher;
    switcher.prod = production;
    return_with murmur_hash(switcher.prod_as_chars, P_SIZE_OF_REWRITE_RULE, 73);
}

/**
 * Allocate a new parser on the heap.
 */
PParser *parser_alloc($) { $H
    PParser *P = mem_alloc(sizeof(PParser));

    if(NULL == P) {
        mem_error("Unable to allocate a new parser on the heap.");
    }

    P->productions = dict_alloc($$A 10, &P_hash_production_fnc);
    P->rules = dict_alloc($$A 15, &P_hash_rewrite_rule_fnc);

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
                           PParserRuleSequence *arg1, ...) { /* rewrite rules */
    va_list seqs;

    $H
    assert_not_null(P);
    assert_not_null(semantic_handler_fnc);
    assert(0 < num_seqs);
    assert_not_null(arg1);
    assert(0 == P->is_closed);
    assert(NULL == dict_get($$A P->productions, semantic_handler_fnc));

    PParserRuleSequence *curr_seq;
    P_RuleSequenceSequence *S = NULL,
                           *curr = NULL,
                           *tail = NULL;

    va_start(seqs, arg1);
    for(curr_seq = arg1; \
        num_seqs > 0; \
        --num_seqs, curr_seq = va_arg(seqs, PParserRuleSequence *)) {

        curr = (P_RuleSequenceSequence *) list_alloc($$A sizeof(P_RuleSequenceSequence));
        curr->seq = curr_seq;

        /* add this into the sequence */
        if(NULL != tail) {
            list_set_next($$A tail, curr);

        /* no tail ==> need to set the head of the list. */
        } else {
            S = curr;
        }

        tail = curr;
    }

    /* add in this production */
    dict_set($$A P->productions, semantic_handler_fnc, S, &delegate_do_nothing);

    return_with;
}

/**
 * Create one of the rule sequences needed for a production.
 */
PParserRuleSequence *parser_rule_sequence($$ short num_rules, PParserRewriteRule *arg1, ...) { $H
    va_list rules;

    $H;
    assert(0 < num_rules);
    assert_not_null(arg1);

    PParserRewriteRule *curr_rule;
    PParserRuleSequence *S = NULL,
                        *curr = NULL,
                        *tail = NULL;

    va_start(rules, arg1);
    for(curr_rule = arg1; \
        num_rules > 0; \
        --num_rules, curr_rule = va_arg(rules, PParserRewriteRule *)) {

        curr = (PParserRuleSequence *) list_alloc($$A sizeof(PParserRuleSequence));
        curr->rule = curr_rule;

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
    PParserRewriteRule rewrite_rule,
                       *R;

    rewrite_rule.func = func;
    rewrite_rule.lexeme = tok;

    R = dict_get($$A P->rules, &rewrite_rule);
    if(NULL != R) {
        return_with R;
    }

    // nope, need to allocate it :(
    R = mem_alloc(sizeof(PParserRewriteRule));
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
