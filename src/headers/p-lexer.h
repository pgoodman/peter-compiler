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

typedef enum {
    P_LEXEME_EPSILON,

    P_LEXEME_ADD,
    P_LEXEME_MULTIPLY,
    P_LEXEME_PAREN_OPEN,
    P_LEXEME_PAREN_CLOSE,
    P_LEXEME_NUMBER
/*
    P_LEXEME_EMPTY_STRING,
    P_LEXEME_TERMINAL,
    P_LEXEME_NON_TERMINAL,
    P_LEXEME_SEQUENCE_OPEN,
    P_LEXEME_SEQUENCE_CLOSE,
    P_LEXEME_ORDERED_CHOICE,
    P_LEXEME_GREEDY_REPETITION,
    P_LEXEME_GREEDY_POSITIVE_REPETITION,
    P_LEXEME_OPTIONAL,
    P_LEXEME_FOLLOWED_BY,
    P_NOT_FOLLOWED_BY
*/
} PLexeme;

typedef struct PToken {
    PLexeme lexeme;
    PString *val;
    uint32_t line,
             start_column,
             end_column;
} PToken;

typedef struct PTokenGenerator {
    PGenerator _;
} PTokenGenerator;

#endif /* PLEXER_H_ */
