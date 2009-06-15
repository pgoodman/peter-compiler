/*
 * p-frame.h
 *
 *  Created on: Jun 2, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PFRAME_H_
#define PFRAME_H_

#include "std-include.h"
#include "p-types.h"

#include "p-cache.h"
#include "p-tree.h"



P_ProductionStack *P_production_alloc(void);

void P_Production_free(P_ProductionStack *stack);

/* TODO WTF GCC not allowing P_TreeSet for all_trees */
void P_production_push(P_ProductionStack *stack,
                       P_ProductionCache *cache,
                       PDictionary *all_trees,
                       G_ProductionRule *prod,
                       PTerminalTree *backtrack_point);

void P_production_push_lr(P_ProductionStack *stack,
                          P_ProductionCache *cache,
                          PDictionary *all_trees,
                          G_ProductionRule *prod,
                          PTerminalTree *backtrack_point);

void P_production_succeeded(P_ProductionStack *stack,
                            P_ProductionCache *cache,
                            PTerminalTree *end_point);

void P_production_failed(P_ProductionStack *stack,
                         P_ProductionCache *cache);

void P_production_cache_succeeded(P_ProductionStack *stack,
                                  P_ProductionCacheValue *cached_result);

void P_production_continue(P_ProductionStack *stack);

void P_production_break(P_ProductionStack *stack);

void P_production_cascade(P_ProductionStack *stack, P_ProductionCache *cache);

int P_production_rule_failed(P_ProductionStack *stack);

int P_production_can_cascade(P_ProductionStack *stack);

int P_production_is_root(P_ProductionStack *stack);

PTerminalTree *P_production_backtrack(P_ProductionStack *stack);

int P_production_can_backtrack(P_ProductionStack *stack);

PParserRewriteRule *P_production_get_rule(P_ProductionStack *stack);

int P_production_has_rule(P_ProductionStack *stack);

PParseTree *P_production_get_parse_tree(P_ProductionStack *stack);

P_ProductionCacheValue *P_production_get_cache(P_ProductionStack *stack,
                                               P_ProductionCache *cache);

void P_production_grow_lr(P_ProductionStack *stack,
                          P_ProductionCache *cache,
                          P_ProductionCacheValue *cached_result,
                          PTerminalTree **curr);

unsigned long int prod_num_allocated_pointers(void);

#endif /* PFRAME_H_ */
