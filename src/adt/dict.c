/*
 * hash-set.c
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-dict.h>

#define P_HASH_TABLE_LOAD_FACTOR 0.65

typedef union {
    void *ptr;
    char chars[sizeof(void *) / sizeof(char)];
} pointer_to_char_t;

/**
 * Credit for primes table: Aaron Krowne
 * http://br.endernet.org/~akrowne/
 * http://planetmath.org/encyclopedia/GoodHashTablePrimes.html
 */
static const unsigned int primes[] = {
    53, 97, 193, 389,
    769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869,
    3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741
};

static const int num_primes = sizeof(primes)/sizeof(primes[0]);

/**
 * Allocate the slots used by a vector.
 */
static PDictEntry **H_alloc_slots(uint32_t num_slots ) { $H
    PDictEntry **slots = mem_calloc(num_slots, sizeof(PDictEntry *));

    if(is_null(slots)) {
        mem_error("Unable to allocate hash table slots.");
    }

    return_with slots;
}

/**
 * Double the size of the hash table and re-hash all the values.
 */
static void H_grow(PDictionary *H ) { $H
    assert_not_null(H);

    PDictEntry **slots,
               **elms = H->elms;

    if(num_primes < ++(H->prime_index)) {
        std_error("Error: Unable to grow hash table further.");
    }

    uint32_t old_size = H->num_slots,
             new_size = primes[H->prime_index],
             j;

    /* don't perform any resize operation */
    slots = H_alloc_slots(new_size);

    /* re-hash the old objects into the new table */
    for(j = 0; j < old_size; ++j) {
        slots[H->key_hash_fnc(elms[j]->key) % new_size] = elms[j];
    }

    /* free the old memory and update our hash table */
    mem_free(elms);

    H->elms = slots;
    H->num_slots = new_size;

    return_with;
}

/**
 * Allocate a generic hash table on the heap. A generic hash table allows the
 * programmer to augment the base hash table data structure.
 */
void *gen_dict_alloc(const size_t dict_struct_size,
                     uint32_t num_slots,
                     PHashFunction key_hash_fnc,
                     PHashCollisionFunction val_collision_fnc) {
    $H
    assert(sizeof(PDictionary) <= dict_struct_size);
    assert_not_null(key_hash_fnc);
    assert_not_null(val_collision_fnc);

    PDictionary *H;
    int i;

    void *table = mem_alloc(dict_struct_size);
    if(is_null(table)) {
        mem_error("Unable to allocate vector on the heap.");
    }

    /* figure out the minimum size we will use from our prime table. */
    for(i = 0; i < num_primes; ++i) {
        if(num_slots < primes[i]) {
            num_slots = primes[i];
            break;
        }
    }

    if(num_primes < i) {
        std_error("Error, the hash table size given is too large.");
    }

    PDictEntry **elms = H_alloc_slots(num_slots);

    /* initialize the vector */
    H = (PDictionary *) table;
    H->elms = elms;
    H->num_slots = num_slots;
    H->num_used_slots = 0;
    H->key_hash_fnc = key_hash_fnc;
    H->collision_fnc = val_collision_fnc;
    H->prime_index = i;

    return_with table;
}

/**
 * Allocate a hash table on the heap.
 */
PDictionary *dict_alloc(const uint32_t num_slots,
                        PHashFunction key_hash_fnc,
                        PHashCollisionFunction collision_fnc) {
    $H
    return_with (PDictionary *) gen_dict_alloc(
        sizeof(PDictionary),
        num_slots,
        key_hash_fnc,
        collision_fnc
    );
}

/**
 * Free a hash table.
 */
void dict_free(PDictionary *H, PDelegate free_elm_fnc ) { $H
    assert_not_null(H);
    assert_not_null(free_elm_fnc);

    uint32_t i;

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

    return_with;
}

/**
 * Set a record into a hash table. This will return 1 if a previous element
 * existed, 0 if it did not. In any case, the value will be set.
 */
char dict_set(PDictionary *H, void *key, void *val,
              PDelegate free_on_overwrite_fnc) { $H

    assert_not_null(H);
    assert_not_null(key);
    assert_not_null(val);
    assert_not_null(free_on_overwrite_fnc);

    /* we've gone past our load factor, grow the hash table. */
    if(P_HASH_TABLE_LOAD_FACTOR < (H->num_used_slots / H->num_slots)) {
        H_grow(H);
    }

    uint32_t hash_key = (H->key_hash_fnc(key) % H->num_slots);
    char did_overwrite = 0;
    PDictEntry *X = H->elms[hash_key];

    /* overwrite with our object, but only if the keys match! */
    if(is_not_null(X)) {

        /* linear probing */
        while(is_not_null(X) && H->collision_fnc(X->key, key)) {
            hash_key = (hash_key+1) % H->num_slots;
            X = H->elms[hash_key];
        }

        if(is_not_null(X)) {
            free_on_overwrite_fnc(H->elms[hash_key]);
            did_overwrite = 1;
        }
    }

    X = mem_alloc(sizeof(PDictEntry));
    if(is_null(X)) {
        mem_error("Unable to allocate dictionary entry on the heap.");
    }

    X->entry = val;
    X->key = key;

    H->elms[hash_key] = X;

    return_with did_overwrite;
}

/**
 * Delete a record from a hash table.
 */
void dict_unset(PDictionary *H, void *key, PDelegate free_fnc ) { $H
	assert_not_null(H);
	assert_not_null(key);

    uint32_t h = H->key_hash_fnc(key) % H->num_slots;

    /* remove it if it is there and decrement the number of slots
     * that we're using. */
    if(is_not_null(H->elms[h])) {
        free_fnc(H->elms[h] );
        H->elms[h] = NULL;
        --(H->num_used_slots);
    }

    return_with;
}

/**
 * Get a record from a hash table.
 */
void *dict_get(const PDictionary * const H, const void * const key) { $H
    assert_not_null(H);


    uint32_t hash_key = H->key_hash_fnc((void *) key) % H->num_slots;
    PDictEntry *X = H->elms[hash_key];

    /* re-apply the hash function until we find what we're looking for. */
    while(is_not_null(X) && H->collision_fnc((void *) key, X->key)) {
        hash_key = (hash_key+1) % H->num_slots;
        X = H->elms[hash_key];
    }

    return_with (is_null(X) ? NULL : X->entry);
}

/**
 * Check if a record exists in a hash table.
 */
inline char dict_is_set(const PDictionary * const H, const void * const key) {
    return is_not_null(dict_get(H, key));
}

/**
 * Turn a pointer into an array of char and then hash it.
 */
uint32_t dict_hash_pointer(void *pointer ) { $H
    pointer_to_char_t switcher;
    switcher.ptr = pointer;
    return_with murmur_hash(switcher.chars, 4, 73);
}
