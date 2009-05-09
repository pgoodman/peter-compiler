/*
 * hash-set.c
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "hash-table.h"

#define HASH_TABLE_LOAD_FACTOR 0.7

/**
 * Allocate the slots used by a vector.
 */
static void **H_alloc_slots(uint32_t num_slots) {
    void **slots = mem_calloc(num_slots, sizeof(void *));
    if(NULL == slots)
        mem_error("Unable to allocate hash table slots.");

    return slots;
}

/**
 * Double the size of the hash table and re-hash all the values.
 */
static void H_grow(HashTable *H) {
    assert(NULL != H);

    void **slots,
         **elms = H->elms;

    uint32_t old_size = H->num_slots,
             new_size,
             j;

    if(old_size == 0xFFFFFFFF)
        std_error("Error: Unable to grow hash table further.");

    new_size = old_size > 0x7FFFFFFF ? 0xFFFFFFFF : old_size * 2;

    hash_func_t hash_fnc = H->hash_fnc;

    /* don't perform any resize operation */
    slots = H_alloc_slots(new_size);

    /* re-hash the old objects into the new table */
    for(j = 0; j < old_size; ++j)
        slots[hash_fnc(elms[j], new_size)] = elms[j];

    /* free the old memory and update our hash table */
    mem_free(elms);

    H->elms = slots;
    H->num_slots = new_size;
}

/**
 * Allocate a generic hash table on the heap.
 */
void *gen_hash_table_alloc(size_t size, const uint32_t num_slots) {
    uint32_t i;
    void **elms,
         *table;
    HashTable *H;

    if(size < sizeof(HashTable))
        size = sizeof(HashTable);

    table = mem_alloc(size);
    if(NULL == table)
        mem_error("Unable to allocate vector on the heap.");

    elms = H_alloc_slots(num_slots);

    /* initialize the vector */
    H = (HashTable *) table;
    H->elms = elms;
    H->num_slots = num_slots;
    H->num_used_slots = 0;

    /* null out all of the slots */
    for(i = 0; i < num_slots; ++i)
        elms[i] = NULL;

    return table;
}

/**
 * Allocate a hash table on the heap.
 */
HashTable *hash_table_alloc(const uint32_t num_slots) {
    return (HashTable *) gen_hash_table_alloc(0, num_slots);
}

/**
 * Set a record into a hash table.
 */
void hash_table_set(HashTable *H, void *key, void *val, D1_t free_on_overwrite_fnc) {
    assert(NULL != H && NULL != key && NULL != val && free_on_overwrite_fnc != NULL);

    uint32_t h;

    /* we've gone past our load factor, grow the hash table. */
    if(HASH_TABLE_LOAD_FACTOR < (H->num_used_slots / H->num_slots))
        H_grow(H);

    /* add in our object */
    h = H->hash_fnc(key, H->num_slots);

    if(NULL != H->elms[h])
        free_on_overwrite_fnc(H->elms[h]);

    H->elms[h] = val;
}

/**
 * Delete a record from a hash table.
 */
void hash_table_unset(HashTable *H, void *c, D1_t free_fnc) {
    assert(NULL != H && NULL != c);

    uint32_t h = H->hash_fnc(c, H->num_slots);

    /* remove it if it is there and decrement the number of slots
     * that we're using. */
    if(NULL != H->elms[h]) {
        free_fnc(H->elms[h]);
        H->elms[h] = NULL;
        --(H->num_used_slots);
    }
}
