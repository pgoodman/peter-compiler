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
#include "p-common-types.h"

/* base parse types, holds our rewrite rules. */
typedef struct PParser {

    /* on for when we no longer allow rules to be added. */
    char is_closed;

    /* keep track of all of the productions for the parsing grammar. */
    P_Production *productions;
    unsigned int num_productions,
                 num_tokens;

    /* keep track of rules used in productions, this does not keep track of
     * epsilon rules as that is statically defined anyway. */
    PDictionary *rules;

    /* production used to start parsing. */
    unsigned char start_production;
} PParser;

/* types relating to how the internal call stack is re-written using these
 * rules.
 */
typedef struct PParserRewriteRule {
    unsigned char production,
                  token,
                  flag;
} PParserRewriteRule;

/* the result of creating a parser rewrite rule. */
typedef struct PParserRuleResult {
    PGenericList *rule;
    unsigned short num_useful_elms;
} PParserRuleResult;

#endif /* PPARSERTYPES_H_ */
