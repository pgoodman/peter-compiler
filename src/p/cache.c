/*
 * thunk.c
 *
 *  Created on: Jun 2, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-cache.h>

#define C_SIZE_OF_KEY (sizeof(P_ProductionCacheKey) / sizeof(char))
#define C_SIZE_OF_VAL (sizeof(P_ProductionCacheValue) / sizeof(char))
#define C_SIZE_OF_KEYVAL ((sizeof(P_ProductionCacheKey) + sizeof(P_ProductionCacheValue)) / sizeof(char))

#define P_PRODUCTION_FAILED ((void *) 10)
#define P_PRODUCTION_INITIAL ((void *) 11)

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
        char chars[C_SIZE_OF_KEY];
    } switcher;
    assert_not_null(thunk);

    /* first clear out any random stack garbage left from previous calls */
    for(i = 0; i < C_SIZE_OF_KEY; ++i) {
        switcher.chars[i] = 0;
    }

    /* make the thunk :D */
    switcher.thunk.terminal_tree = thunk->terminal_tree;
    switcher.thunk.production = thunk->production;

    return murmur_hash(switcher.chars, C_SIZE_OF_KEY, 73);
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
    char *data;
    P_ProductionCacheKey *key;
    P_ProductionCacheValue *val;

    assert_not_null(cache);
    assert_null(P_cache_get(cache, production, start_position));

    /* allocate the cache key and value at once */
    data = cache_mem_calloc(C_SIZE_OF_KEYVAL, sizeof(char));
    if(is_null(data)) {
        mem_error("Unable to allocate parser cache entry.");
    }

    /* initialize the key/value pair */
    key = (P_ProductionCacheKey *) data;
    val = (P_ProductionCacheValue *) (data + C_SIZE_OF_VAL);

    key->production = production;
    key->terminal_tree = start_position;

    val->end = NULL;
    val->is_left_recursive = 0;
    val->tree = P_PRODUCTION_INITIAL;

    /* set the cache entry */
    dict_set(
        cache,
        key,
        val,
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
    P_ProductionCacheValue *value;
    assert_not_null(cache);
    value = P_cache_get(cache, production, start_position);
    assert_not_null(value);
    value->end = end_position;
    value->tree = parse_tree;
}

/**
 * Cache a failed result in the thunk table.
 */
void P_cache_store_failure(P_ProductionCache *cache,
                           unsigned char production,
                           PTerminalTree *start_position) {
    P_cache_store_success(
        cache,
        production,
        start_position,
        NULL,
        P_PRODUCTION_FAILED
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
PParseTree *P_cache_value_get_tree(const P_ProductionCacheValue *val) {
    assert_not_null(val);
    assert(val->tree != P_PRODUCTION_FAILED);
    assert(val->tree != P_PRODUCTION_INITIAL);

    return val->tree;
}

/**
 * Get where the cache value parsed to.
 */
PTerminalTree *P_cache_value_get_terminal(const P_ProductionCacheValue *val) {
    assert_not_null(val);
    assert(val->tree != P_PRODUCTION_FAILED);
    assert(val->tree != P_PRODUCTION_INITIAL);
    return val->end;
}

/**
 * Return whether or not a cached result is a failure.
 */
int P_cache_value_is_failure(const P_ProductionCacheValue *val) {
    assert_not_null(val);
    return val->tree == P_PRODUCTION_FAILED;
}

/**
 * Return whether or not a cached result is has been initialized but not yet
 * had a result stored to it.
 */
int P_cache_value_is_missing(const P_ProductionCacheValue *val) {
    assert_not_null(val);
    return val->tree == P_PRODUCTION_INITIAL;
}

/**
 * Set this cached result to be a left-recursive result.
 */
void P_cache_value_enable_left_recursion(P_ProductionCacheValue *val) {
    assert_not_null(val);
    assert(val->tree != P_PRODUCTION_FAILED);
    val->is_left_recursive = 1;
    val->tree = P_PRODUCTION_FAILED;
}

/**
 * Set this cached result to be a non-left-recursive result.
 */
void P_cache_value_disable_left_recursion(P_ProductionCacheValue *val) {
    assert_not_null(val);
    val->is_left_recursive = 0;
}

/**
 * Check if a chached result is left recursive.
 */
int P_cache_value_is_left_recursive(const P_ProductionCacheValue *val) {
    assert_not_null(val);
    return (int) val->is_left_recursive;
}
