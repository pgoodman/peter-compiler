/*
 * thunk.c
 *
 *  Created on: Jun 2, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-cache.h>

#define P_SIZE_OF_CACHE_KEY (sizeof(P_ProductionCacheKey) / sizeof(char))
#define P_PRODUCTION_FAILED ((void *) 10)

#define cache_mem_alloc(x) mem_alloc(x); ++num_allocations
#define cache_mem_calloc(x,y) mem_calloc(x,y); ++num_allocations
#define cache_mem_free(x) mem_free(x); --num_allocations
#define cache_mem_error(x) mem_error(x)

static unsigned long int num_allocations = 0;
unsigned long int cache_num_allocated_pointers(void) {
    return num_allocations;
}

/**
 * Hash function that converts a thunk to a char array. Thunks are used as the
 * keys into a hash table for cached parse results.
 */
static uint32_t C_key_hash_fnc(P_ProductionCacheKey *thunk) {
    unsigned int i;

    union {
        P_ProductionCacheKey thunk;
        char chars[P_SIZE_OF_CACHE_KEY];
    } switcher;
    assert_not_null(thunk);

    /* first clear out any random stack garbage left from previous calls */
    for(i = 0; i < P_SIZE_OF_CACHE_KEY; ++i) {
        switcher.chars[i] = 0;
    }

    /* make the thunk :D */
    switcher.thunk.terminal_tree = thunk->terminal_tree;
    switcher.thunk.production = thunk->production;

    return murmur_hash(switcher.chars, P_SIZE_OF_CACHE_KEY, 73);
}

/**
 * Check for thunk collisions.
 */
static char C_key_collision_fnc(P_ProductionCacheKey *t1,
                                P_ProductionCacheKey *t2) {
    return ((t1->terminal_tree != t2->terminal_tree)
        || (t1->production != t2->production));
}

/**
 * Free a cached key.
 */
static void C_key_free(P_ProductionCacheKey *key) {
    assert_not_null(key);
    cache_mem_free(key);
}

/**
 * Free a cached result.
 */
static void C_value_free(P_ProductionCacheValue *val) {
    assert_not_null(val);
    if(P_PRODUCTION_FAILED != val) {
        cache_mem_free(val);
    }
}

/**
 * Allocate a new thunk on the heap.
 */
static P_ProductionCacheKey *C_key_alloc(unsigned char production,
                                         PTerminalTree *terminal_tree) {

    P_ProductionCacheKey *thunk = cache_mem_alloc(sizeof(P_ProductionCacheKey));
    if(is_null(thunk)) {
        mem_error("Unable to heap allocate a thunk.");
    }

    thunk->terminal_tree = terminal_tree;
    thunk->production = production;

    return thunk;
}

/**
 * Allocate a new cached result of parsing on the heap.
 */
static P_ProductionCacheValue *C_value_alloc(PTerminalTree *end,
                                             PParseTree *tree) {

    P_ProductionCacheValue *val = cache_mem_alloc(sizeof(P_ProductionCacheValue));
    if(is_null(val)) {
        mem_error("Unable to cache the result of a production on the heap.");
    }

    val->end = end;
    val->tree = tree;
    val->is_left_recursive = 0;

    return val;
}

/**
 * Allocate a new cache.
 */
P_ProductionCache *P_cache_alloc(uint32_t num_tokens) {
    return dict_alloc(
        num_tokens,
        (PDictionaryHashFunc) &C_key_hash_fnc,
        (PDictionaryCollisionFunc) &C_key_collision_fnc
    );
}

/**
 * Free a parser cache.
 */
void P_cache_free(P_ProductionCache *cache) {

    dict_free(
        cache,
        (PDelegate) &C_key_free,
        (PDelegate) &C_value_free
    );
}

static uint32_t hash_it(P_ProductionCacheKey *key) {
    uint32_t k = C_key_hash_fnc(key);
    return k;
}


/**
 * Store the initial cache value for a production. This way we only allocate
 * cache keys once.
 */
void P_cache_store_initial(P_ProductionCache *cache,
                           unsigned char production,
                           PTerminalTree *start_position) {

    assert_not_null(cache);
    assert_null(P_cache_get(cache, production, start_position));

    dict_set(
        cache,
        C_key_alloc(
            production,
            start_position
        ),
        P_PRODUCTION_FAILED,
        (PDelegate) &C_value_free
    );
}

/**
 * Cache a successful application of a production to the token stream.
 */
void P_cache_store_success(P_ProductionCache *cache,
                           unsigned char production,
                           PTerminalTree *start_position,
                           PTerminalTree *end_position,
                           PParseTree *parse_tree) {
    P_ProductionCacheKey thunk;

    assert_not_null(cache);
    assert_not_null(P_cache_get(cache, production, start_position));

    thunk.terminal_tree = start_position;
    thunk.production = production;

    dict_set(
        cache,
        &thunk,
        C_value_alloc(
            end_position,
            parse_tree
        ),
        (PDelegate) &C_value_free
    );
}

/**
 * Cache a failed result in the thunk table.
 */
void P_cache_store_failure(P_ProductionCache *cache,
                           unsigned char production,
                           PTerminalTree *start_position) {
    P_ProductionCacheKey thunk;

    assert_not_null(cache);
    assert_not_null(P_cache_get(cache, production, start_position));

    thunk.terminal_tree = start_position;
    thunk.production = production;

    dict_set(
        cache,
        &thunk,
        P_PRODUCTION_FAILED,
        (PDelegate) &C_value_free
    );
}

/**
 * Get a cached result.
 */
P_ProductionCacheValue *P_cache_get(P_ProductionCache *cache,
                                    unsigned char production,
                                    PTerminalTree *start_position) {
    P_ProductionCacheKey thunk;

    assert_not_null(cache);

    thunk.terminal_tree = start_position;
    thunk.production = production;

    return dict_get(cache, &thunk);
}

/**
 * Get the parse tree stored in a cache value.
 */
PParseTree *P_cache_value_get_tree(P_ProductionCacheValue *val) {
    assert_not_null(val);
    assert(val != P_PRODUCTION_FAILED);

    return val->tree;
}

/**
 * Get where the cache value parsed to.
 */
PTerminalTree *P_cache_value_get_terminal(P_ProductionCacheValue *val) {
    assert_not_null(val);
    assert(val != P_PRODUCTION_FAILED);
    return val->end;
}

/**
 * Return whether or not a cached result is a failure.
 */
int P_cache_value_is_failure(P_ProductionCacheValue *result) {
    assert_not_null(result);
    return result == P_PRODUCTION_FAILED;
}
