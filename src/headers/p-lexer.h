/*
 * p-lexer.h
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PLEXER_H_
#define PLEXER_H_

typedef enum {
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
} PLexeme;

typedef struct PToken {
    PLexeme _;
    PString *val;
} PToken;

#endif /* PLEXER_H_ */