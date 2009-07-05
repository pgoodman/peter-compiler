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
#include "p-scanner.h"

PT_Terminal *PT_alloc_terminal(G_Terminal terminal,
                               PString *lexeme,
                               uint32_t line,
                               uint32_t column,
                               uint32_t id);

PT_NonTerminal *PT_alloc_non_terminal(G_NonTerminal production,
                                      unsigned short num_branches);

PT_Epsilon *PT_alloc_epsilon(void);

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
