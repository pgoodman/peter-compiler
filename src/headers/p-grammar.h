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

void grammar_add_production_rule(PGrammar *grammar,
                                 G_NonTerminal production);

void grammar_add_phrase(PGrammar *grammar);

void grammar_add_non_terminal_symbol(PGrammar *grammar,
                                     G_NonTerminal production,
                                     G_TreeOp tree_op);

void grammar_add_terminal_symbol(PGrammar *grammar,
                                 G_Terminal token,
                                 G_TreeOp tree_op);

void grammar_add_cut_symbol(PGrammar *grammar);

void grammar_add_fail_symbol(PGrammar *grammar);

void grammar_add_epsilon_symbol(PGrammar *grammar, G_TreeOp tree_op);

short grammar_get_num_production_rules(PGrammar *grammar);

void grammar_null_action(void *s, unsigned char r, unsigned int n, PParseTree *c[]);

void grammar_add_tree_actions(PGrammar *grammar,
                              PTreeTraversalType traversal_type,
                              G_ProductionRuleFunc *actions[]);

void grammar_add_state_action(PGrammar *grammar, PDelegate *action_fnc);

#endif /* P_GRAMMAR_H_ */
