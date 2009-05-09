/*
 * hash-set.h
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef HASHSET_H_
#define HASHSET_H_

#include <stdheader.h>
#include "list.h"
#include "vector.h"

/* hash functions take in an object to hash as well as the
 * current size of the hash table.
 */
typedef uint32_t (*hash_func_t)(void *, uint32_t);

/* Hash table / set implementation. */
typedef struct HashTable {
    void ** elms;

    uint32_t num_slots,
             num_used_slots;

    hash_func_t hash_fnc;
} HashTable;

void *gen_hash_table_alloc(size_t, const uint32_t);
HashTable *hash_table_alloc(const uint32_t);
void hash_table_set(HashTable *, void *, void *, D1_t);
void hash_table_unset(HashTable *, void *, D1_t);

#endif /* HASHSET_H_ */
