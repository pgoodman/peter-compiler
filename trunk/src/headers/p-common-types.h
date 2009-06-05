/*
 * p-prod-common.h
 *
 *  Created on: May 23, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PPRODCOMMON_H_
#define PPRODCOMMON_H_

#include "p-lexer.h"
#include "adt-tree.h"

/* parse tree, only used for productions, leaves are tokens :) */
typedef enum {
    P_PARSE_TREE_PRODUCTION,
    P_PARSE_TREE_TERMINAL,
    P_PARSE_TREE_EPSILON
} PParseTreeType;

typedef struct PParseTree {
    PTree _;
    PParseTreeType type;
} PParseTree;

typedef struct PProductionTree {
    PParseTree _;
    unsigned char production,
                  /* the rule from the production's definition that matched */
                  rule;
} PProductionTree;

typedef struct PTerminalTree {
    PParseTree _;
    PToken *token;
    struct PTerminalTree *prev,
                         *next;
} PTerminalTree;

/**
 * Type representing a single production and all of its rules 'alternatives'
 * from a top-down parsing grammar. The list is of rules is explicitly ordered.
 *
 * The 'max_rule_elms' is maximum number of non/terminals in all of its rules.
 * This is used to allocate one and only one parse tree with max_rule_elms
 * branch pointers.
 */
typedef struct PParserProduction {
    PGenericList *alternatives;
    unsigned char production;
    unsigned short max_num_useful_rewrite_rules;
} PParserProduction;

#endif /* PPRODCOMMON_H_ */
