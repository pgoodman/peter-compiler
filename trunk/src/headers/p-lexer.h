/*
 * p-lexer.h
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PLEXER_H_
#define PLEXER_H_

#include "adt-generator.h"
#include "adt-dict.h"
#include "std-string.h"
#include "std-input.h"

typedef struct PToken {
    char token;
    PString *lexeme;
    uint32_t line,
             column;
} PToken;

typedef struct PTokenGenerator {
    PGenerator _;
    PFileInputStream *stream;
    uint32_t line,
             column;
    char start_char;
} PTokenGenerator;

PToken *token_alloc(char token, PString *lexeme, uint32_t line, uint32_t col);

void token_free(PToken *tok);

PTokenGenerator *token_generator_alloc(const char *, PFunction);

unsigned long int token_num_allocated_pointers(void);

#endif /* PLEXER_H_ */
