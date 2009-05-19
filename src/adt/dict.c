/*
 * hash-set.c
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-dict.h>

#define P_DICT_LOAD_FACTOR 0.65
#define P_DICT_EMPTY_SLOT (void *)1

/**
 * Credit for primes table: Aaron Krowne
 * http://br.endernet.org/~akrowne/
 * http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
 */
static const unsigned int H_primes[] = {
    53, 97, 193, 389,
    769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869,
    3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741
};

static const int H_num_primes = sizeof(H_primes)/sizeof(H_primes[0]);

typedef struct H_EntryInfo {
    H_Entry *entry;
    uint32_t hash_key;
} H_EntryInfo;

/**
 * Locate an entry and its associated key into the hash table in a dictionary.
 */
static H_EntryInfo H_get_entry_info(PDictionary *H, void *key) {
    H_EntryInfo info;

    assert_not_null(H);

    info.hash_key = H->key_hash_fnc((void *) key) % H->num_slots;
    info.entry = H->elms[info.hash_key];

    /* re-apply the hash function until we find what we're looking for. */
    while(is_not_null(info.entry) && H->collision_fnc((void *) key, info.entry->key)) {
        info.hash_key = (info.hash_key+1) % H->num_slots;
        info.entry = H->elms[info.hash_key];
    }

    return info;
}

/**
 * Allocate the slots used by a vector.
 */
static H_Entry **H_alloc_slots(uint32_t num_slots ) {
    H_Entry **slots = mem_calloc(num_slots, sizeof(H_Entry *));
    if(is_null(slots)) {
        mem_error("Unable to allocate hash table slots.");
    }
    return slots;
}

/**
 * Double the size of the hash table and re-hash all the values.
 */
static void H_grow(PDictionary *H ) {
    H_Entry **old_elms = NULL;
    H_EntryInfo info;
    uint32_t old_size,
             j;

    assert_not_null(H);

    if(H_num_primes < ++(H->prime_index)) {
        std_error("Error: Unable to grow hash table further.");
    }

    old_elms = H->elms;
    old_size = H->num_slots;

    H->num_slots = H_primes[H->prime_index];
    H->elms = H_alloc_slots(H->num_slots);

    /* re-hash the old objects into the new table */
    for(j = 0; j < old_size; ++j) {
        info = H_get_entry_info(H, old_elms[j]->key);
        H->elms[info.hash_key] = old_elms[j];
    }

    /* free the old memory and update our hash table */
    mem_free(old_elms);
    return;
}

/**
 * Allocate a generic hash table on the heap. A generic hash table allows the
 * programmer to augment the base hash table data structure.
 */
void *gen_dict_alloc(const size_t dict_struct_size,
                     uint32_t num_slots,
                     PDictHashFunction key_hash_fnc,
                     PDictCollisionFunction val_collision_fnc) {
    PDictionary *H;
    H_Entry **elms;
    int i;
    void *table;

    assert(sizeof(PDictionary) <= dict_struct_size);
    assert_not_null(key_hash_fnc);
    assert_not_null(val_collision_fnc);

    table = mem_alloc(dict_struct_size);
    if(is_null(table)) {
        mem_error("Unable to allocate vector on the heap.");
    }

    /* figure out the minimum size we will use from our prime table. */
    for(i = 0; i < H_num_primes; ++i) {
        if(num_slots < H_primes[i]) {
            num_slots = H_primes[i];
            break;
        }
    }

    if(H_num_primes < i) {
        std_error("Error, the hash table size given is too large.");
    }

    elms = H_alloc_slots(num_slots);

    /* initialize the vector */
    H = (PDictionary *) table;
    H->elms = elms;
    H->num_slots = num_slots;
    H->num_used_slots = 0;
    H->key_hash_fnc = key_hash_fnc;
    H->collision_fnc = val_collision_fnc;
    H->prime_index = i;

    return table;
}

/**
 * Allocate a hash table on the heap.
 */
PDictionary *dict_alloc(const uint32_t num_slots,
                        PDictHashFunction key_hash_fnc,
                        PDictCollisionFunction collision_fnc) {
    return (PDictionary *) gen_dict_alloc(
        sizeof(PDictionary),
        num_slots,
        key_hash_fnc,
        collision_fnc
    );
}

/**
 * Free a hash table.
 */
void dict_free(PDictionary *H, PDelegate free_elm_fnc ) {
    uint32_t i;

    assert_not_null(H);
    assert_not_null(free_elm_fnc);

    /* free the elements stored in the hash table. */
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

/**
 * Set a record into a hash table. This will return 1 if a previous element
 * existed, 0 if it did not. In any case, the value will be set.
 */
char dict_set(PDictionary *H, void *key, void *val,
              PDelegate free_on_overwrite_fnc) {

    char did_overwrite;
    H_Entry *entry;
    H_EntryInfo info;

    assert_not_null(H);
    assert_not_null(key);
    assert_not_null(free_on_overwrite_fnc);

    /* we've gone past our load factor, grow the hash table. */
    if(P_DICT_LOAD_FACTOR < (H->num_used_slots / H->num_slots)) {
        H_grow(H);
    }

    info = H_get_entry_info(H, key);
    entry = info.entry;
    did_overwrite = 0;

    /* overwrite with our object, but only if the keys match! */
    if(is_not_null(entry)) {
        free_on_overwrite_fnc(info.entry->entry);
        did_overwrite = 1;
    } else {
        entry = mem_alloc(sizeof(H_Entry));
        if(is_null(entry)) {
            mem_error("Unable to allocate dictionary entry on the heap.");
        }
    }

    entry->entry = val;
    entry->key = key;
    H->elms[info.hash_key] = entry;

    return did_overwrite;
}

/**
 * Delete a record from a hash table.
 */
void dict_unset(PDictionary *H, void *key, PDelegate free_fnc) {
    H_EntryInfo info = H_get_entry_info(H, key);
    uint32_t prev_key;

    /* remove it if it is there and decrement the number of slots
     * that we're using. */
    if(is_not_null(info.entry)) {
        free_fnc(info.entry->entry);
        mem_free(info.entry);
        H->elms[info.hash_key] = NULL;

        /* shift the elements that were linearly probed into place. */
        prev_key = info.hash_key;
        info.hash_key = (info.hash_key + 1) % H->num_slots;
        while(is_not_null(H->elms[info.hash_key])) {
            if(H->collision_fnc(H->elms[info.hash_key]->key, key)) {
                H->elms[prev_key] = H->elms[info.hash_key];
                prev_key = info.hash_key;
                info.hash_key = (info.hash_key + 1) % H->num_slots;
            }
        }

        --(H->num_used_slots);
    }
    return;
}

/**
 * Get a record from a hash table.
 */
void *dict_get(PDictionary *H, void *key) {
    H_EntryInfo info = H_get_entry_info(H, key);
    return (is_null(info.entry) ? NULL : info.entry->entry);
}

/**
 * Check if a record exists in a hash table.
 */
char dict_is_set(PDictionary *H, void *key) {
    H_EntryInfo info = H_get_entry_info(H, key);
    return is_not_null(info.entry);
}

/**
 * Hash a pointer.
 */
uint32_t dict_pointer_hash_fnc(void *pointer) {
    union {
        void *ptr;
        char ptr_as_chars[sizeof(void *) / sizeof(char)];
    } switcher;
    switcher.ptr = pointer;
    return murmur_hash(switcher.ptr_as_chars, (sizeof(void *) / sizeof(char)), 73);
}

/**
 * Check if two pointers are the same.
 */
char dict_pointer_collision_fnc(void *a, void *b) {
    return (a == b);
}
