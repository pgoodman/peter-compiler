/*
 * p-adt.h
 *
 *  Created on: May 19, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PADT_H_
#define PADT_H_

#include "p-parser-types.h"
#include "p-prod-dict.h"
#include "p-parser.h"

PParser *parser_alloc(PParserFunc);
void parser_free(PParser *);
void parser_add_production(PParser *, PParserFunc, short, PParserRuleResult, ...);
PParserRuleResult parser_rule_sequence(short, PParserRewriteRule *, ...);
PParserRewriteRule *parser_rewrite_function(PParser *, PParserFunc);
PParserRewriteRule *parser_rewrite_token(PParser *, char);
PParserRewriteRule *parser_rewrite_epsilon(PParser *);

#endif /* PADT_H_ */
