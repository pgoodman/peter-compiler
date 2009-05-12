/*
 * p-parser.c
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-parser.h>

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
    char thunk_as_chars[ \
        (0
         + sizeof(PParserFunc *) \
         + sizeof(PToken *) \
         + sizeof(PParserTokenList *) \
        ) / sizeof(char) \
    ];
} P_ThunkPointer;

/* call stack type. */
typedef struct P_CallStack {
    PList _;
    short local_extent;
    PParserRewriteRule *rewrite_rule;
};

/* Hash function that converts a thunk to a char array */
static uint32_t P_hash_thunk_fnc(void *pointer $$) { $H
    P_ThunkPointer switcher;
    switcher.thunk = pointer;
    return_with murmur_hash(switcher.thunk_as_chars, 4, 73);
}

PParser *parser_alloc($) {
    PParser *P;

    return_with P;
}
