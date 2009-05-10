/*
 * hash-set.h
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef HASHSET_H_
#define HASHSET_H_

#include "std-include.h"
#include "adt-list.h"
#include "adt-vector.h"
#include "vendor-murmur-hash.h"

/* hash functions take in an object to hash as well as the
 * current size of the hash table.
 */
typedef uint32_t (*PHashFunction)(void * $$);

/* Hash table / set implementation. */
typedef struct PHashTable {
    void ** elms;

    uint32_t num_slots,
             num_used_slots;

    PHashFunction hash_fnc;
} PHashTable;

void *gen_hash_table_alloc(const size_t, const uint32_t, PHashFunction $$);
PHashTable *hash_table_alloc(const uint32_t, PHashFunction $$);
void hash_table_free(PHashTable *, PDelegate $$);
char hash_table_set(PHashTable *, void *, void *, PDelegate $$);
void hash_table_unset(PHashTable *, void *, PDelegate $$);
void *hash_table_get(PHashTable *, void * $$);

uint32_t hash_table_hash_pointer(void * $$);

#endif /* HASHSET_H_ */
