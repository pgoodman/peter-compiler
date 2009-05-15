/*
 * p-parser-types.h
 *
 *  Created on: May 15, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PPARSERTYPES_H_
#define PPARSERTYPES_H_

#include "adt-tree.h"
#include "p-lexer.h"

/* parse tree, only used for productions, leaves are tokens :) */
typedef enum {
    P_PARSE_TREE_PRODUCTION,
    P_PARSE_TREE_TERMINAL
} PParseTreeType;

typedef struct PParseTree {
    PTree _;
    PParseTreeType type;
} PParseTree;

typedef struct PProductionTree {
    PParseTree _;
    struct PParseTree *(*production)(struct PParseTree *);
    short rule; /* the rule from the production's definition that was matched */
} PProductionTree;

typedef struct PTerminalTree {
    PParseTree _;
    PToken *token;
    struct PTerminalTree *prev,
                         *next;
} PTerminalTree;

/* a parser function. a parser function deals with the *semantic* meaning of
 * a particular node in a parse tree. These functions are called *after* the
 * entire and correct parse tree is generated.
 */
typedef PParseTree *(*PParserFunc)(PParseTree *);


/*********************/
typedef uint32_t (*ProdDictionaryHashFunction)(PParserFunc);
typedef char (*ProdDictionaryCollisionFunction)(PParserFunc, PParserFunc);
typedef struct PD_Entry {
    void * entry;
    PParserFunc key;
} PD_Entry;
typedef struct ProdDictionary {
    PD_Entry ** elms;
    uint32_t num_slots,
             num_used_slots;
    ProdDictionaryHashFunction key_hash_fnc;
    ProdDictionaryCollisionFunction collision_fnc;
    int prime_index;
} ProdDictionary;
/*********************/


/* base parse types, holds our rewrite rules. */
typedef struct PParser {
    /*PGenericList *parsing_grammar;*/
    PDictionary *thunk_table;

    char is_closed; /* on for when we no longer allow rules to be added. */

    ProdDictionary *productions; /* keep track of all of the productions for the
                                  * parsing grammar. */

    PDictionary *rules; /* keep track of rules used in productions, this does
                         * not keep track of epsilon rules as that is statically
                         * defined anyway. */

    PStack *temp_parse_trees; /* unused or cached parsed trees. */

    PParserFunc start_production; /* production used to start parsing. */
} PParser;

/* types relating to how the internal call stack is re-written using these
 * rules.
 */
typedef struct PParserRewriteRule {
    PParserFunc func;
    PLexeme lexeme;
} PParserRewriteRule;

/* the result of creating a parser rewrite rule. */
typedef struct PParserRuleResult {
    PGenericList *rule;
    short num_elms;
} PParserRuleResult;

#endif /* PPARSERTYPES_H_ */