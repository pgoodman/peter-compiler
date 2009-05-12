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

#define P_SIZE_OF_REWRITE_RULE ((sizeof(PParserRewriteFunc) > sizeof(PParserRewriteToken)) \
        ? sizeof(PParserRewriteFunc) \
        : sizeof(PParserRewriteToken))

/* type describing a type used to store a lazy result for a function */
typedef struct PParserThunk {
    PParserFunc *func;
    PToken *car;
    PParserTokenList *cdr;
} P_Thunk;

/* type converter for out lazy func result type to a char array for our hash
 * function.
 */
typedef union {
    P_Thunk thunk;
    char thunk_as_chars[P_SIZE_OF_THUNK];
} P_ThunkPointer;

/* call stack type. */
typedef struct P_CallStack {
    PList _;
    short local_extent;
    PParserRewriteRule *rewrite_rule;
} P_CallStack;

/* Hash function that converts a thunk to a char array */
static uint32_t P_hash_thunk_fnc($$ void *pointer ) { $H
    P_ThunkPointer switcher;
    switcher.thunk = *((P_Thunk *) pointer);
    return_with murmur_hash(switcher.thunk_as_chars, P_SIZE_OF_THUNK, 10);
}

/**
 * Allocate a new parser on the heap.
 */
PParser *parser_alloc($) { $H
    PParser *P = mem_alloc(sizeof(PParser));

    if(NULL == P) {
        mem_error("Unable to allocate a new parser on the heap.");
    }

    P->rewrite_rules = gen_list_alloc($A);

    return_with P;
}

/**
 * Add a production to the parser's grammar. The production's name is the name
 * (or rather the function pointer) of the parser function that handles semantic
 * actions on the parse tree. This function pointer is used to reference this
 * production in the rules.
 *
 * !!! Rules *must* be null-terminated!
 */
void parser_add_production($$ PParser *P, short num_rules,
                           PParserFunc semantic_handler_fnc,
                           PParserRewriteRule *arg1, ...) {
    va_list rules;

    $H
    assert_not_null(P);
    assert_not_null(semantic_handler_fnc);

    PParserRewriteRule *curr_rule,
                       *temp,
                       *final_rules;

    /* allocate heap space for this array of rules */
    final_rules = mem_alloc(P_SIZE_OF_REWRITE_RULE);

    if(NULL == final_rules) {
        mem_error("Unable to allocate space for the production rules on the heap.");
    }

    va_start(rules, arg1);

    for(curr_rule = arg1; \
         num_rules > 0; \
         --num_rules, curr_rule = va_arg(rules, PParserRewriteRule *)) {

        /* figure out how many rules there are in this set */
        for(temp = curr_rule; NULL != temp; ++temp)
            ;

        /* allocate some rules! */

    }
    va_end(rules);
}
