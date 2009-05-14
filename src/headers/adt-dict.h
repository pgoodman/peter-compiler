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
typedef uint32_t (*PHashFunction)(void *);

/* hash collision checker. first parameter is a key of one element, the other is
 * the value of another. */
typedef char (*PHashCollisionFunction)(void *, void *);

/* Hash table / set implementation. */
typedef struct PDictionary {
    void ** elms;

    uint32_t num_slots,
             num_used_slots;

    PHashFunction key_hash_fnc,
                  val_hash_fnc;

    PHashCollisionFunction collision_fnc;

    int prime_index;
} PDictionary;

void *gen_dict_alloc(const size_t, const uint32_t, PHashFunction, PHashFunction, PHashCollisionFunction);
PDictionary *dict_alloc(const uint32_t, PHashFunction, PHashFunction, PHashCollisionFunction);
void dict_free(PDictionary *, PDelegate );
char dict_set(PDictionary *, void *, void *, PDelegate );
void dict_unset(PDictionary *, void *, PDelegate );

void *dict_get(const PDictionary * const H, const void * const key);
inline char dict_is_set(const PDictionary * const H, const void * const key);

uint32_t dict_hash_pointer(void * );

#endif /* HASHSET_H_ */
