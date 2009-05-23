/*
 * p-parser-types.h
 *
 *  Created on: May 15, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PPARSERTYPES_H_
#define PPARSERTYPES_H_


#include "p-lexer.h"
#include "p-prod-common.h"
#include "p-prod-dict.h"

#define P_LEXEME_EPSILON -1

/* base parse types, holds our rewrite rules. */
typedef struct PParser {

    /* on for when we no longer allow rules to be added. */
    char is_closed;

    /* keep track of all of the productions for the parsing grammar. */
    PParserProdDictionary *productions;

    /* keep track of rules used in productions, this does not keep track of
     * epsilon rules as that is statically defined anyway. */
    PDictionary *rules;

    /* production used to start parsing. */
    PParserFunc start_production;
} PParser;

/* types relating to how the internal call stack is re-written using these
 * rules.
 */
typedef struct PParserRewriteRule {
    PParserFunc func;
    char lexeme;
} PParserRewriteRule;

/* the result of creating a parser rewrite rule. */
typedef struct PParserRuleResult {
    PGenericList *rule;
    short num_elms;
} PParserRuleResult;

#endif /* PPARSERTYPES_H_ */
