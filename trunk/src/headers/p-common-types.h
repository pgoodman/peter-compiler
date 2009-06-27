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

/* -------------------------------------------------------------------------- */

/**
 * Parse Tree Types
 */

/* The type of a parse tree. */
typedef enum {
    PT_NON_TERMINAL,
    PT_TERMINAL,
    PT_EPSILON
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

    unsigned char phrase;

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

/* -------------------------------------------------------------------------- */

typedef void (G_ProductionRuleFunc)(void *state,
                                    unsigned char phrase,
                                    unsigned int num_children,
                                    PParseTree *children[]);

typedef struct G_Symbol {
    union {
        G_NonTerminal non_terminal;
        G_Terminal terminal;
    } value;

    unsigned int is_non_terminal:1,
                 is_terminal:1,
                 is_epsilon_transition:1,
                 is_non_excludable:1,
                 children_must_be_raised:1,
                 is_fail:1,
                 is_cut:1;
} G_Symbol;

typedef struct G_Phrase {
    G_Symbol *symbols;
    unsigned int num_symbols;
} G_Phrase;

typedef struct G_ProductionRule {
    G_Phrase *phrases;
    G_ProductionRuleFunc *action_fnc;
    G_NonTerminal production;
    unsigned int num_phrases;
} G_ProductionRule;

#endif /* PPRODCOMMON_H_ */
