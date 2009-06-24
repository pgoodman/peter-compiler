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
#include "vendor-murmur-hash.h"

typedef void * H_key_type;
typedef void * H_val_type;

/* hash functions take in an object to hash as well as the
 * current size of the hash table.
 */
typedef uint32_t (*H_hash_fnc_type)(H_key_type);
typedef char (*H_collision_fnc_type)(H_key_type, H_key_type);
typedef void (*H_free_val_fnc_type)(H_val_type);
typedef void (*H_free_key_fnc_type)(H_key_type);

/* hash table entry, this is a private type; however, it needs to be out here */
typedef struct H_Entry {
    struct H_Entry *next;
    H_val_type entry;
    H_key_type key;
} H_Entry;

/* Hash table / set implementation. */
typedef struct {
    H_Entry **slots;
    uint32_t num_slots,
             num_used_slots;
    H_hash_fnc_type key_hash_fnc;
    H_collision_fnc_type collision_fnc;
    unsigned char prime_index,
                  grow_table;
} H_type;

void *gen_dict_alloc(const size_t dict_struct_size,
                     uint32_t num_slots,
                     H_hash_fnc_type key_hash_fnc,
                     H_collision_fnc_type key_collision_fnc);

H_type *dict_alloc(const uint32_t num_slots,
                   H_hash_fnc_type key_hash_fnc,
                   H_collision_fnc_type collision_fnc);

void dict_free(H_type *H,
               H_free_key_fnc_type free_key_fnc,
               H_free_val_fnc_type free_entry_fnc);

void dict_set(H_type *H,
              H_key_type key,
              H_val_type val,
              H_free_val_fnc_type free_on_overwrite_fnc);

void dict_unset(H_type *H,
                H_key_type key,
                H_free_key_fnc_type free_key_fnc,
                H_free_val_fnc_type free_val_fnc);

H_val_type dict_get(H_type *H, H_key_type key);

char dict_is_set(H_type *H, H_key_type key);

uint32_t dict_size(H_type *H);

/* this is the type to be used by outside programs */
typedef H_type PDictionary;
typedef H_hash_fnc_type PDictionaryHashFunc;
typedef H_collision_fnc_type PDictionaryCollisionFunc;
typedef H_free_val_fnc_type PDictionaryFreeValueFunc;

/* generic helper functions for simple key types */
uint32_t dict_pointer_hash_fnc(void *pointer);
char dict_pointer_collision_fnc(void *a, void *b);

unsigned long int dict_num_allocated_pointers(void);

#endif /* HASHSET_H_ */
