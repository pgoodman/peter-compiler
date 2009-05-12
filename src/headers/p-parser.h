/*
 * p-parser.h
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PPARSER_H_
#define PPARSER_H_

#include "adt-tree.h"
#include "adt-list.h"
#include "p-lexer.h"

/* base parse types, holds our rewrite rules. */
typedef struct PParser {
    PGenericList *rewrite_rules;

} PParser;

/* a full list of all of the tokens in a file. */
typedef struct PParserTokenList {
    PList _;
    PToken token;
} PParserTokenList;

/* a parse tree, constructed by the parser automatically */
typedef struct PParseTree {
    PTree _;
} PParseTree;

/* a parser function. a parser function deals with the *semantic* meaning of
 * a particular node in a parse tree. These functions are called *after* the
 * entire and correct parse tree is generated.
 */
typedef PParseTree *(*PParserFunc)(PParseTree *);

/* types relating to how the internal call stack is re-written using these
 * rules.
 */
typedef enum {
    P_PARSER_REWRITE_FUNCTION,
    P_PARSER_REWRITE_TOKEN
} PParserRewriteRuleType;

typedef struct PParserRewriteRule {
    PParserRewriteRuleType type;
} PParserRewriteRule;

typedef struct PParserRewriteFunc {
    PParserRewriteRule _;
    PParserFunc *func;
} PParserRewriteFunc;

typedef struct PParserRewriteToken {
    PParserRewriteRule _;
    PToken *tok;
} PParserRewriteToken;

#endif /* PPARSER_H_ */