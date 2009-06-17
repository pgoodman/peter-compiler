/*
 * p-adt.h
 *
 *  Created on: May 19, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef P_GRAMMAR_H_
#define P_GRAMMAR_H_

#include "p-types.h"
#include "p-parser.h"

PGrammar *grammar_alloc(G_NonTerminal start_production,
                        short num_productions,
                        short num_tokens,
                        short num_phrases,
                        short num_symbols);

void grammar_free(PGrammar *grammar);

void grammar_add_production_rule(PGrammar *grammar, G_NonTerminal production);

void grammar_add_phrase(PGrammar *grammar);

void grammar_add_non_terminal_symbol(PGrammar *grammar,
                                     G_NonTerminal production,
                                     unsigned char is_non_excludable,
                                     unsigned char must_be_raised);

void grammar_add_terminal_symbol(PGrammar *grammar,
                                 G_Terminal token,
                                 unsigned char is_non_excludable);

void grammar_add_epsilon_symbol(PGrammar *grammar,
                                unsigned char is_non_excludable);

short grammar_get_num_production_rules(PGrammar *grammar);

#endif /* P_GRAMMAR_H_ */
