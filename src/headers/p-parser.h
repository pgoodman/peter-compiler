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
#include <unistd.h>

#include "std-include.h"
#include "adt-list.h"
#include "adt-dict.h"
#include "func-delegate.h"

#include "p-parser-types.h"
#include "adt-typesafe-prod-dict.h"

PParser *parser_alloc(PParserFunc);
void parser_add_production(PParser *, PParserFunc, short, PParserRuleResult, ...);
PParserRuleResult parser_rule_sequence(short, PParserRewriteRule *, ...);
PParserRewriteRule *parser_rewrite_function(PParser *, PParserFunc);
PParserRewriteRule *parser_rewrite_token(PParser *, PLexeme);
PParserRewriteRule *parser_rewrite_epsilon(PParser *);
void parser_parse_tokens(PParser *, PTokenGenerator *);

#endif /* PPARSER_H_ */
