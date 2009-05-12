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
 * !!! This function cannot use the stack trace macro functionality as it makes
 *     use of variadic arguments.
 */
void parser_add_production($$ PParser *P, PParserFunc semantic_handler_fnc, PParserRewriteRule **rules, ...) { $H

}
