/*
 * p-thunk.h
 *
 *  Created on: Jun 2, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PTHUNK_H_
#define PTHUNK_H_

#include "std-include.h"
#include "adt-dict.h"
#include "p-types.h"

typedef PDictionary P_ProductionCache;

/* Type representing the necessary information to uniquely identify the result
 * of a production after being applied to a particular suffix of tokens. In this
 * case, list is some pointer to a generic list of tokens that the production was
 * applied to. The token list is ordered and never changes order and so the
 * result of a production to some part in the token list can be meaningfully
 * cached.
 */
typedef struct P_ProductionCacheKey {
    unsigned char production;
    PTerminalTree *terminal_tree;
} P_ProductionCacheKey;

/**
 * Type representing the cached result of the application of a production to
 * some part in the token list 'start'. This type is only used on successful
 * parse, the P_PRODUCTION_FAILED pointer is instead cached to indicate a failed
 * application of a production to some part of the token list.
 *
 * 'end' points to somewhere in the token list for the parser to pick up after
 * accepting the cached parse tree, 'tree', from the application of this
 * production.
 */
typedef struct P_ProductionCacheValue {
    PTerminalTree *end;
    PParseTree *tree;
    char is_left_recursive;
} P_ProductionCacheValue;

P_ProductionCache *P_cache_alloc(uint32_t num_tokens);

void P_cache_free(P_ProductionCache *cache);

void P_cache_store_success(P_ProductionCache *thunk_table,
                           unsigned char production,
                           PTerminalTree *start_position,
                           PTerminalTree *end_position,
                           PParseTree *parse_tree);

void P_cache_store_initial(P_ProductionCache *cache,
                           unsigned char production,
                           PTerminalTree *start_position);

void P_cache_store_failure(P_ProductionCache *thunk_table,
                           unsigned char production,
                           PTerminalTree *start_position);

P_ProductionCacheValue *P_cache_get(P_ProductionCache *thunk_table,
                                    unsigned char production,
                                    PTerminalTree *start_position);

int P_cache_value_is_failure(const P_ProductionCacheValue *result);

int P_cache_value_is_missing(const P_ProductionCacheValue *result);

PParseTree *P_cache_value_get_tree(const P_ProductionCacheValue *val);

PTerminalTree *P_cache_value_get_terminal(const P_ProductionCacheValue *val);

void P_cache_value_enable_left_recursion(P_ProductionCacheValue *result);

void P_cache_value_disable_left_recursion(P_ProductionCacheValue *result);

int P_cache_value_is_left_recursive(const P_ProductionCacheValue *result);

unsigned long int cache_num_allocated_pointers(void);

#endif /* PTHUNK_H_ */
