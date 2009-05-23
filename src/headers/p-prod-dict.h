/*
 * hash-set.h
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef HASHSET_PD_
#define HASHSET_PD_

#include "std-include.h"
#include "vendor-murmur-hash.h"
#include "p-prod-common.h"

typedef PParserFunc PD_key_type;
typedef PParserProduction * PD_val_type;

/* hash functions take in an object to hash as well as the
 * current size of the hash table.
 */
typedef uint32_t (*PD_hash_fnc_type)(PD_key_type);

/* hash collision checker, compares two keys. */
typedef char (*PD_collision_fnc_type)(PD_key_type, PD_key_type);

typedef void (*PD_free_val_fnc_type)(PD_val_type);
typedef void (*PD_free_key_fnc_type)(PD_key_type);

/* hash table entry, this is a private type; however, it needs to be out here */
typedef struct {
    PD_val_type entry;
    PD_key_type key;
} PD_Entry;

/* Hash table / set implementation. */
typedef struct {
    PD_Entry ** elms;
    uint32_t num_slots,
             num_used_slots;
    PD_hash_fnc_type key_hash_fnc;
    PD_collision_fnc_type collision_fnc;
    short prime_index;
} PD_type;

void *gen_prod_dict_alloc(const size_t prod_dict_struct_size,
                     uint32_t num_slots,
                     PD_hash_fnc_type key_hash_fnc,
                     PD_collision_fnc_type val_collision_fnc);

PD_type *prod_dict_alloc(const uint32_t num_slots,
                   PD_hash_fnc_type key_hash_fnc,
                   PD_collision_fnc_type collision_fnc);

void prod_dict_free(PD_type *H,
               PD_free_val_fnc_type free_entry_fnc,
               PD_free_key_fnc_type free_key_fnc);

char prod_dict_set(PD_type *H,
              PD_key_type key,
              PD_val_type val,
              PD_free_val_fnc_type free_on_overwrite_fnc);

void prod_dict_unset(PD_type *H,
                PD_key_type key,
                PD_free_val_fnc_type free_val_fnc,
                PD_free_key_fnc_type free_key_fnc);

PD_val_type prod_dict_get(PD_type *H, PD_key_type key);

char prod_dict_is_set(PD_type *H, PD_key_type key);

/* this is the type to be used by outside programs */
typedef PD_type PParserProdDictionary;

#endif /* HASHSET_PD_ */
