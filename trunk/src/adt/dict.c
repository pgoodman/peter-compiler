/*
 * hash-set.c
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-dict.h>

#define P_HASH_TABLE_LOAD_FACTOR 0.7

typedef union {
    void *ptr;
    char chars[sizeof(void *) / sizeof(char)];
} pointer_to_char_t;

/**
 * Allocate the slots used by a vector.
 */
static void **H_alloc_slots(uint32_t num_slots ) { $H
    void **slots = mem_calloc(num_slots, sizeof(void *));

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

    void **slots,
         **elms = H->elms;

    uint32_t old_size = H->num_slots,
             new_size,
             j;

    if(old_size == 0xFFFFFFFF) {
        std_error("Error: Unable to grow hash table further.");
    }

    new_size = old_size > 0x7FFFFFFF ? 0xFFFFFFFF : old_size * 2;

    PHashFunction hash_fnc = H->val_hash_fnc;

    /* don't perform any resize operation */
    slots = H_alloc_slots(new_size );

    /* re-hash the old objects into the new table */
    for(j = 0; j < old_size; ++j) {
        slots[hash_fnc(elms[j] ) % new_size] = elms[j];
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
                     const uint32_t num_slots,
                     PHashFunction key_hash_fnc,
                     PHashFunction val_hash_fnc) {

    $H
    assert(sizeof(PDictionary) <= dict_struct_size);
    assert_not_null(key_hash_fnc);
    assert_not_null(val_hash_fnc)

    PDictionary *H;
    uint32_t actual_slots;

    void *table = mem_alloc(dict_struct_size);
    if(is_null(table)) {
        mem_error("Unable to allocate vector on the heap.");
    }

    /* find out the number of slots such that we can fill num_slots in O(1)
     * time without needing to resize the hash table. */
    actual_slots = (uint32_t) ((num_slots / P_HASH_TABLE_LOAD_FACTOR) + 1);

    void **elms = H_alloc_slots(num_slots );

    /* initialize the vector */
    H = (PDictionary *) table;
    H->elms = elms;
    H->num_slots = num_slots;
    H->num_used_slots = 0;
    H->key_hash_fnc = key_hash_fnc;
    H->val_hash_fnc = val_hash_fnc;

    return_with table;
}

/**
 * Allocate a hash table on the heap.
 */
PDictionary *dict_alloc(const uint32_t num_slots,
                        PHashFunction key_hash_fnc,
                        PHashFunction val_hash_fnc) {
    $H
    return_with (PDictionary *) gen_dict_alloc(
        sizeof(PDictionary),
        num_slots,
        key_hash_fnc,
        val_hash_fnc
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
        H_grow(H );
    }

    uint32_t hashed_key = H->key_hash_fnc(key ) % H->num_slots;
    char did_overwrite = 0;

    /* add in our object */
    if(is_not_null(H->elms[hashed_key])) {
        free_on_overwrite_fnc(H->elms[hashed_key] );
        did_overwrite = 1;
    }

    H->elms[hashed_key] = val;

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
void *dict_get(PDictionary *H, void *key ) { $H
    assert_not_null(H);
    void *X = H->elms[(H->key_hash_fnc(key ) % H->num_slots)];
    return_with (NULL == X ? NULL : X);
}

/**
 * Check if a record exists in a hash table.
 */
char dict_is_set(PDictionary *H, void *key) { $H
    assert_not_null(H);
    return_with (NULL != (H->elms[(H->key_hash_fnc(key) % H->num_slots)]));
}

/**
 * Turn a pointer into an array of char and then hash it.
 */
uint32_t dict_hash_pointer(void *pointer ) { $H
    pointer_to_char_t switcher;
    switcher.ptr = pointer;
    return_with murmur_hash(switcher.chars, 4, 73);
}
