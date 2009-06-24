/*
 * p-tree.h
 *
 *  Created on: Jun 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PTREE_H_
#define PTREE_H_

#include "std-include.h"
#include "p-types.h"

PT_NonTerminal *PT_alloc_non_terminal(G_NonTerminal production,
                                      unsigned short num_branches);

PT_Epsilon *PT_alloc_epsilon(void);

PT_Terminal *PT_alloc_terminals(PScanner *scanner,
                                PScannerFunction *scanner_fnc,
                                PT_Set *tree_set);

/* -------------------------------------------------------------------------- */

void parse_tree_free(PParseTree *parse_tree);

void parse_tree_print_dot(PParseTree *parse_tree,
                          char production_names[][40],
                          char terminal_names[][40]);

/* -------------------------------------------------------------------------- */

PT_Set *PTS_alloc(void);

void PTS_free(PT_Set *set);

void PTS_add(PT_Set *set, PParseTree *tree);

void PTS_remove(PT_Set *set, PParseTree *tree);

uint32_t PTS_size(PT_Set *set);

#endif /* PTREE_H_ */
