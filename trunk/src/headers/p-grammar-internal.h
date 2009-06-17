/*
 * p-grammar-internal.h
 *
 *  Created on: May 19, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PGRAMMAR_INTERNAL_H
#define PGRAMMAR_INTERNAL_H

#include "p-types.h"
#include "p-parser.h"

void G_lock(PGrammar *grammar);

int G_production_rule_has_phrase(G_ProductionRule *, unsigned int phrase);

G_Symbol *G_production_rule_get_symbol(G_ProductionRule *,
                                       unsigned int phrase,
                                       unsigned int symbol);

int G_symbol_is_non_excludable(G_Symbol *symbol);

int G_symbol_use_children_instead(G_Symbol *symbol);

int G_symbol_is_non_terminal(G_Symbol *symbol);

int G_symbol_is_terminal(G_Symbol *symbol);

int G_symbol_is_epsilon(G_Symbol *symbol);

#endif /* PGRAMMAR_INTERNAL_H */
