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

/**
 * In a sense this parser is recursive-descent; however, it maintains its own
 * heap-allocated stack frames, as opposed to calling production functions
 * recursively.
 *
 * The choice to manage stack frames explicitly has a few very nice benefits:
 *   i) it allows for the functions that perform the actual parsing to be
 *      isolated and for the operations of the parser to be clearly defined and
 *      predictable.
 *   ii) it allows for the actual parser to be defined entirely in terms of
 *       primitive pattern matching against the grammar as opposed to requiring
 *       independent functions to be constructed to perform the actual parsing.
 *   iii) it allows for a one-to-one correspondence between the definition of
 *        the parser in C and the top-down parsing language grammar.
 *
 * A stack frame holds important information, namely:
 *    a) the current production in being executed.
 *    b) a list of the remaining non/terminals to match against tokens in the
 *       token list. When this list is NULL it implies that we have reached the
 *       end of it and thus have matched all of the non/terminals of this
 *       particular production's rule.
 *    c) the alternative rules if any non/terminal in the curr_rule_list fails
 *       to match. If the alternative_rules list is null and the 'do_backtrack'
 *       flag is set then it implies that all of the rules for the current
 *       production have failed to match the tokens.
 *    d) the backtrack_point points to where in the token list the parser was
 *       when it attempted to start matching this production. This is the point
 *       where it will return to on failure in order to try matching another
 *       rule.
 *    e) a do_backtrack flag indicating that a non/terminal in
 *       curr_rule_list has failed to match a token and so we must
 *       backtrack to the backtrack_point and try another rule, or upon
 *       full failure of the production, cascade the failure to the
 *       calling production.
 *    f) the parse tree as it is being constructed.
 *    g) the calling stack frame. This is used for two reasons, it lets us
 *       manually manage the call stack without the overhead of the generic
 *       stack data structure and it also lets us reclaim stack frames for
 *       future use.
 */
typedef struct P_StackFrame {
    PParserProduction *production;
    PGenericList *curr_rule_list, /* list of rewrite rules */
                 *alternative_rules; /* list of lists of rewrite rules */
    PTerminalTree *backtrack_point;
    char do_backtrack;
    PParseTree *parse_tree;
    struct P_StackFrame *caller;
} P_StackFrame;

/* Manage used and unused stack frames. */
typedef struct P_ProductionStack {
    P_StackFrame *frame,
                 *unused;
} P_ProductionStack;

P_ProductionStack *P_production_alloc(void);

void P_Production_free(P_ProductionStack *stack);

/* TODO WTF GCC not allowing P_TreeSet for all_trees */
void P_production_push(P_ProductionStack *stack,
                       P_ProductionCache *cache,
                       PDictionary *all_trees,
                       PParserProduction *prod,
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

int P_production_rule_failed(P_ProductionStack *stack);

int P_production_can_cascade(P_ProductionStack *stack);

int P_production_is_root(P_ProductionStack *stack);

PTerminalTree *P_production_backtrack(P_ProductionStack *stack);

int P_production_can_backtrack(P_ProductionStack *stack);

PParserRewriteRule *P_production_get_rule(P_ProductionStack *stack);

int P_production_has_rule(P_ProductionStack *stack);

PParseTree *P_production_get_parse_tree(P_ProductionStack *stack);


unsigned long int prod_num_allocated_pointers(void);

#endif /* PFRAME_H_ */
