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
#include "func-delegate.h"
#include "vendor-murmur-hash.h"

/* hash functions take in an object to hash as well as the
 * current size of the hash table.
 */
typedef uint32_t (*PDictHashFunction)(void *);

/* hash collision checker, compares two keys. */
typedef char (*PDictCollisionFunction)(void *, void *);

/* hash table entry, this is a private type; however, it needs to be out here */
typedef struct H_Entry {
    void *entry,
         *key;
} H_Entry;

/* Hash table / set implementation. */
typedef struct PDictionary {
    H_Entry ** elms;

    uint32_t num_slots,
             num_used_slots;

    PDictHashFunction key_hash_fnc;

    PDictCollisionFunction collision_fnc;

    int prime_index;
} PDictionary;

void *gen_dict_alloc(const size_t, const uint32_t, PDictHashFunction, PDictCollisionFunction);
PDictionary *dict_alloc(const uint32_t, PDictHashFunction, PDictCollisionFunction);
void dict_free(PDictionary *, PDelegate );
char dict_set(PDictionary *, void *, void *, PDelegate );
void dict_unset(PDictionary *, void *, PDelegate );

void *dict_get(PDictionary *H, void *key);
char dict_is_set(PDictionary *H, void *key);

uint32_t dict_pointer_hash_fnc(void *pointer);
char dict_pointer_collision_fnc(void *a, void *b);

#endif /* HASHSET_H_ */
