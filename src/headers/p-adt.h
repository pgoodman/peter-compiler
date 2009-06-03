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
#include "p-parser.h"

PParser *parser_alloc(unsigned char start_production,
                      size_t num_productions,
                      size_t num_tokens);

void parser_free(PParser *P);

void parser_add_production(PParser *P,
                           unsigned char semantic_handler_fnc,
                           short num_seqs,
                           PParserRuleResult arg1, ...);

PParserRuleResult parser_rule_sequence(PParser *P,
                                       short num_rules,
                                       PParserRewriteRule *arg1, ...);

PParserRewriteRule *parser_rewrite_production(PParser *P,
                                              unsigned char prod,
                                              unsigned char flag);

PParserRewriteRule *parser_rewrite_token(PParser *P,
                                         unsigned char tok,
                                         unsigned char flag);

PParserRewriteRule *parser_rewrite_epsilon(PParser *P,
                                           unsigned char flag);

int parser_rule_is_production(PParserRewriteRule *rule);

int parser_rule_is_token(PParserRewriteRule *rule);

int parser_rule_is_epsilon(PParserRewriteRule *rule);

int parser_rule_is_non_excludable(PParserRewriteRule *rule);

int parser_rule_subsume(PParserRewriteRule *rule);

#endif /* PADT_H_ */
