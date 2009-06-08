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
#include "adt-dict.h"
#include "p-adt.h"

typedef PDictionary P_TreeSet;

/**
 * Type holding a pointer to the ordered list of *all* tokens from whatever text
 * is currently being parsed, as well as the number of tokens in that list. Right
 * off the bat we allocate the leaves of the parse tree and put the tokens in
 * them so that no tree leaf for a given token is allocated more than once. This
 * also gives us the benefit of having a well-defined in-order traversal path in
 * the final parse tree.
 */
typedef struct P_TerminalTreeList {
    PTerminalTree *list;
    uint32_t num_tokens;
} P_TerminalTreeList;

P_TreeSet *P_tree_set_alloc(void);
void P_tree_set_free(P_TreeSet *set);
void P_tree_set_add(P_TreeSet *set, PParseTree *tree);
void P_tree_set_remove(P_TreeSet *set, PParseTree *tree);

PParseTree *P_tree_alloc_epsilon(void);
PProductionTree *P_tree_alloc_non_terminal(unsigned short num_branches,
                                           unsigned char production);
P_TerminalTreeList P_tree_alloc_terminals(P_TreeSet *all_trees,
                                          PTokenGenerator *gen);

void P_tree_record_production(PParseTree *temp_tree,
                              PParseTree *parse_tree,
                              PParserRewriteRule *curr_rule);

int P_tree_progress_was_made(PTerminalTree *tree,
                             unsigned int line,
                             unsigned int column);

void parser_free_parse_tree(PParseTree *tree);

#endif /* PTREE_H_ */
