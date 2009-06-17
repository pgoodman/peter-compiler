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

PT_Terminal *PT_alloc_terminals(PT_Set *all_trees,
                                PToken tokens[],
                                int num_tokens);

void PT_add_branch(PParseTree *parse_tree,
                   PParseTree *branch_tree,
                   G_Symbol *symbol);

void parser_free_parse_tree(PParseTree *parse_tree);

void PT_free_intermediate(PParseTree *parse_tree);

/* -------------------------------------------------------------------------- */

PT_Set *PTS_alloc(void);

void PTS_free(PT_Set *set);

void PTS_add(PT_Set *set, PParseTree *tree);

void PTS_remove(PT_Set *set, PParseTree *tree);

#endif /* PTREE_H_ */
