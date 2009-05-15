/*
 * p-parser.h
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PPARSER_H_
#define PPARSER_H_

#include <stdarg.h>
#include "adt-tree.h"
#include "adt-list.h"
#include "adt-dict.h"
#include "adt-tree.h"
#include "func-delegate.h"
#include "p-lexer.h"

#include <unistd.h>

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
} PTerminalTree;

/* a parser function. a parser function deals with the *semantic* meaning of
 * a particular node in a parse tree. These functions are called *after* the
 * entire and correct parse tree is generated.
 */
typedef PParseTree *(*PParserFunc)(PParseTree *);

/* base parse types, holds our rewrite rules. */
typedef struct PParser {
    /*PGenericList *parsing_grammar;*/
    PDictionary *thunk_table;

    char is_closed; /* on for when we no longer allow rules to be added. */

    PDictionary *productions; /* keep track of all of the productions for the
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

PParser *parser_alloc(PParserFunc);
void parser_add_production(PParser *, PParserFunc, short, PParserRuleResult, ...);
PParserRuleResult parser_rule_sequence(short, PParserRewriteRule *, ...);
PParserRewriteRule *parser_rewrite_function(PParser *, PParserFunc);
PParserRewriteRule *parser_rewrite_token(PParser *, PLexeme);
PParserRewriteRule *parser_rewrite_epsilon(PParser *);

PParseTree *parser_parse_tokens(PParser *, PTokenGenerator *);

#endif /* PPARSER_H_ */
