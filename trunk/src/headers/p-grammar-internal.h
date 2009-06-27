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

void G_unlock(PGrammar *grammar);

int G_production_rule_has_phrase(G_ProductionRule *, unsigned int phrase);

G_Symbol *G_production_rule_get_symbol(G_ProductionRule *,
                                       unsigned int phrase,
                                       unsigned int symbol);

#endif /* PGRAMMAR_INTERNAL_H */
