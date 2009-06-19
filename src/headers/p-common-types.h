/*
 * p-prod-common.h
 *
 *  Created on: May 23, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PPRODCOMMON_H_
#define PPRODCOMMON_H_

#include "adt-tree.h"

/* -------------------------------------------------------------------------- */

/**
 * Grammar types
 */

typedef unsigned short G_Terminal;
typedef unsigned short G_NonTerminal;

typedef struct G_Symbol {
    union {
        G_NonTerminal non_terminal;
        G_Terminal terminal;
    } value;
    unsigned char flag;
} G_Symbol;

typedef struct G_Phrase {
    G_Symbol *symbols;
    unsigned int num_symbols;
} G_Phrase;

typedef struct G_ProductionRule {
    G_Phrase *phrases;
    G_NonTerminal production;
    unsigned int num_phrases;
} G_ProductionRule;

/* -------------------------------------------------------------------------- */

/**
 * Parse Tree Types
 */

/* The type of a parse tree. */
typedef enum {
    P_PARSE_TREE_PRODUCTION,
    P_PARSE_TREE_TERMINAL,
    P_PARSE_TREE_EPSILON
} PT_Type;

/* A parse tree. */
typedef struct PParseTree {
    PTree _;
    PT_Type type;
} PParseTree;

/* epsilon tree */
typedef PParseTree PT_Epsilon;

/* a parse tree representing a non-terminal symbol, i.e. the result of applying
 * a production rule to the token stream. */
typedef struct PT_NonTerminal {
    PParseTree _;

    /* the rule from the production's definition that matched */
    G_NonTerminal production;

    unsigned char rule;

} PT_NonTerminal;

/* a parse tree representing a terminal symbol, i.e. a token */
typedef struct PT_Terminal {
    PParseTree _;

    G_Terminal terminal;

    PString *lexeme;

    uint32_t line,
             column;

    uint32_t id;

    struct PT_Terminal *next;
} PT_Terminal;

/* a set of parse trees */
typedef PDictionary PT_Set;

#endif /* PPRODCOMMON_H_ */
