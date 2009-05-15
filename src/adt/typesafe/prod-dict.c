/*
 * prod-dict.c
 *
 *  Created on: May 15, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-typesafe-prod-dict.h>

static const unsigned int PD_primes[] = {
    53, 97, 193, 389,
    769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869,
    3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741
};
static const int PD_num_primes = sizeof(PD_primes)/sizeof(PD_primes[0]);

typedef struct PD_EntryInfo {
    PD_Entry *entry;
    uint32_t hash_key;
} PD_EntryInfo;

static PD_EntryInfo  PD_get_entry_info(ProdDictionary *H, PParserFunc key) {
    PD_EntryInfo info;
    assert_not_null(H);
    info.hash_key = H->key_hash_fnc(key) % H->num_slots;
    info.entry = H->elms[info.hash_key];
    while(is_not_null(info.entry) && H->collision_fnc(key, info.entry->key)) {
        info.hash_key = (info.hash_key+1) % H->num_slots;
        info.entry = H->elms[info.hash_key];
    }
    return info;
}
static PD_Entry **PD_alloc_slots(uint32_t num_slots ) {
    PD_Entry **slots = mem_calloc(num_slots, sizeof(PD_Entry *));
    if(is_null(slots)) {
        mem_error("Unable to allocate hash table slots.");
    }
    return slots;
}
static void PD_grow(ProdDictionary *H ) {
    PD_Entry **old_elms = NULL;
    PD_EntryInfo info;
    uint32_t old_size,
             j;
    assert_not_null(H);
    if(PD_num_primes < ++(H->prime_index)) {
        std_error("Error: Unable to grow hash table further.");
    }
    old_elms = H->elms;
    old_size = H->num_slots;
    H->num_slots = PD_primes[H->prime_index];
    H->elms = PD_alloc_slots(H->num_slots);
    for(j = 0; j < old_size; ++j) {
        info = PD_get_entry_info(H, old_elms[j]->key);
        H->elms[info.hash_key] = old_elms[j];
    }
    mem_free(old_elms);
    return;
}
ProdDictionary *prod_dict_alloc(uint32_t num_slots,
                     ProdDictionaryHashFunction key_hash_fnc,
                     ProdDictionaryCollisionFunction val_collision_fnc) {
    ProdDictionary *H;
    PD_Entry **elms;
    int i;
    ProdDictionary *table;
    assert_not_null(key_hash_fnc);
    assert_not_null(val_collision_fnc);
    table = mem_alloc(sizeof(ProdDictionary));
    if(is_null(table)) {
        mem_error("Unable to allocate vector on the heap.");
    }
    for(i = 0; i < PD_num_primes; ++i) {
        if(num_slots < PD_primes[i]) {
            num_slots = PD_primes[i];
            break;
        }
    }
    if(PD_num_primes < i) {
        std_error("Error, the hash table size given is too large.");
    }
    elms = PD_alloc_slots(num_slots);
    H = (ProdDictionary *) table;
    H->elms = elms;
    H->num_slots = num_slots;
    H->num_used_slots = 0;
    H->key_hash_fnc = key_hash_fnc;
    H->collision_fnc = val_collision_fnc;
    H->prime_index = i;
    return table;
}
void prod_dict_free(ProdDictionary *H, PDelegate free_elm_fnc ) {
    uint32_t i;
    assert_not_null(H);
    assert_not_null(free_elm_fnc);
    if(free_elm_fnc != delegate_do_nothing) {
        for(i = 0; i < H->num_slots; ++i) {
            if(is_not_null(H->elms[i])) {
                free_elm_fnc(H->elms[i] );
            }
        }
    }
    mem_free(H->elms);
    mem_free(H);
    H = NULL;
    return;
}
char prod_dict_set(ProdDictionary *H, PParserFunc key, void * val,
              PDelegate free_on_overwrite_fnc) {
    char did_overwrite;
    PD_Entry *entry;
    PD_EntryInfo info;
    assert_not_null(H);
    assert_not_null(key);
    assert_not_null(free_on_overwrite_fnc);
    if(0.65 < (H->num_used_slots / H->num_slots)) {
        PD_grow(H);
    }
    info = PD_get_entry_info(H, key);
    entry = info.entry;
    did_overwrite = 0;
    if(is_not_null(entry)) {
        free_on_overwrite_fnc(info.entry->entry);
        did_overwrite = 1;
    } else {
        entry = mem_alloc(sizeof(PD_Entry));
        if(is_null(entry)) {
            mem_error("Unable to allocate dictionary entry on the heap.");
        }
    }
    entry->entry = val;
    entry->key = key;
    H->elms[info.hash_key] = entry;
    return did_overwrite;
}
void prod_dict_unset(ProdDictionary *H, PParserFunc key, PDelegate free_fnc) {
    PD_EntryInfo info = PD_get_entry_info(H, key);
    if(is_not_null(info.entry)) {
        free_fnc(info.entry->entry);
        mem_free(info.entry);
        H->elms[info.hash_key] = NULL;
        --(H->num_used_slots);
    }
    return;
}
void *  prod_dict_get(ProdDictionary *H, PParserFunc key) {
    PD_EntryInfo info = PD_get_entry_info(H, key);
    return (is_null(info.entry) ? NULL : info.entry->entry);
}
char prod_dict_is_set(ProdDictionary *H, PParserFunc key) {
    PD_EntryInfo info = PD_get_entry_info(H, key);
    return is_not_null(info.entry);
}
