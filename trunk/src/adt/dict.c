/*
 * hash-set.c
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-dict.h>

#define D_LOAD_FACTOR 0.65

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
static unsigned long int num_allocations = 0;

#define dict_mem_alloc(x) mem_alloc(x); ++num_allocations
#define dict_mem_calloc(x,y) mem_calloc(x,y); ++num_allocations
#define dict_mem_free(x) mem_free(x); --num_allocations
#define dict_mem_error(x) mem_error(x)

unsigned long int dict_num_allocated_pointers(void) {
    return num_allocations;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a new dictionary entry.
 */
static H_Entry *H_entry_alloc(H_key_type key, H_val_type val) {
    H_Entry *entry = dict_mem_alloc(sizeof(H_Entry));
    if(is_null(entry)) {
        dict_mem_error("Unable to allocate dictionary entry on the heap.");
    }

    entry->key = key;
    entry->entry = val;
    entry->next = NULL;

    return entry;
}

/* Set an entry to a dictionary. */
static void H_entry_add(H_type *H, H_Entry *E) {
    uint32_t i = H->key_hash_fnc(E->key) % H->num_slots;
    H_Entry *temp,
            *prev;

    if(is_null(H->slots[i])) {
        H->slots[i] = E;
        E->next = NULL;
    } else {
        E->next = H->slots[i];
        H->slots[i] = E;
    }

}

/**
 * Locate an entry and its associated key into the hash table in a dictionary.
 */
static H_Entry *H_entry_get(H_type *H, H_key_type key) {
    H_Entry *entry;

    assert_not_null(H);

    entry = H->slots[H->key_hash_fnc(key) % H->num_slots];
    for(; is_not_null(entry); entry = entry->next) {
        if(!H->collision_fnc(entry->key, key)) {
            return entry;
        }
    }

    return NULL;
}

/**
 * Allocate the slots used by a vector.
 */
static H_Entry **H_slots_alloc(uint32_t num_slots ) {
    H_Entry **slots = dict_mem_calloc(num_slots, sizeof(H_Entry *));
    if(is_null(slots)) {
        dict_mem_error("Unable to allocate hash table slots.");
    }
    return slots;
}

/**
 * Double the size of the hash table and re-hash all the values.
 */
static void H_slots_grow(H_type *H ) {
    H_Entry **old_elms = NULL,
            *entry,
            *next;
    uint32_t old_size,
             j;

    assert_not_null(H);

    if(H_num_primes < ++(H->prime_index)) {
        std_error("Error: Unable to grow hash table further.");
    }

    old_elms = H->slots;
    old_size = H->num_slots;

    H->num_slots = H_primes[(unsigned int) H->prime_index];
    H->slots = H_slots_alloc(H->num_slots);

    /* re-hash the old objects into the new table */
    for(j = 0; j < old_size; ++j) {
        for(entry = old_elms[j]; is_not_null(entry); entry = next) {
            next = entry->next;
            H_entry_add(H, entry);
        }
    }

    dict_mem_free(old_elms);

    H->grow_table = 0;

    return;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a generic hash table on the heap. A generic hash table allows the
 * programmer to augment the base hash table data structure.
 */
void *gen_dict_alloc(const size_t dict_struct_size,
                     uint32_t num_slots,
                     H_hash_fnc_type key_hash_fnc,
                     H_collision_fnc_type key_collision_fnc) {
    H_type *H;
    H_Entry **elms;
    int i;
    void *table;

    assert(sizeof(H_type) <= dict_struct_size);
    assert_not_null(key_hash_fnc);
    assert_not_null(key_collision_fnc);

    table = dict_mem_alloc(dict_struct_size);
    if(is_null(table)) {
        dict_mem_error("Unable to allocate vector on the heap.");
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

    elms = H_slots_alloc(num_slots);

    /* initialize the vector */
    H = (H_type *) table;
    H->slots = elms;
    H->num_slots = num_slots;
    H->num_used_slots = 0;
    H->key_hash_fnc = key_hash_fnc;
    H->collision_fnc = key_collision_fnc;
    H->prime_index = i;
    H->grow_table = 0;

    return table;
}

/**
 * Allocate a hash table on the heap.
 */
H_type *dict_alloc(const uint32_t num_slots,
                   H_hash_fnc_type key_hash_fnc,
                   H_collision_fnc_type key_collision_fnc) {
    return (H_type *) gen_dict_alloc(
        sizeof(H_type),
        num_slots,
        key_hash_fnc,
        key_collision_fnc
    );
}

/**
 * Free a hash table.
 */
void dict_free(H_type *H,
               H_free_key_fnc_type free_key_fnc,
               H_free_val_fnc_type free_val_fnc) {

    uint32_t i;
    H_Entry *entry,
            *next;

    assert_not_null(H);
    assert_not_null(free_val_fnc);
    assert_not_null(free_key_fnc);

    /* free the elements stored in the hash table. */
    for(i = 0; i < H->num_slots; ++i) {
        for(entry = H->slots[i]; is_not_null(entry); entry = next) {
            next = entry->next;

            free_key_fnc(entry->key);
            free_val_fnc(entry->entry);

            dict_mem_free(entry);
        }
    }

    dict_mem_free(H->slots);
    dict_mem_free(H);
    return;
}

/**
 * Set a record into a hash table. This will return 1 if a previous element
 * existed, 0 if it did not. In any case, the value will be set.
 */
void dict_set(H_type *H,
              H_key_type key,
              H_val_type val,
              H_free_val_fnc_type free_val_fnc) {

    uint32_t i;
    H_Entry *entry;

    assert_not_null(H);
    assert_not_null(key);
    assert_not_null(free_val_fnc);

    if(H->grow_table) {
        H_slots_grow(H);
    }

    i = H->key_hash_fnc(key) % H->num_slots;

    if(is_null(H->slots[i])) {
        entry = H_entry_alloc(key, val);
    } else {

        /* look through the bucket to see if we need to overwrite an existing
         * entry. */
        for(entry = H->slots[i]; is_not_null(entry); entry = entry->next) {
            if(!H->collision_fnc(entry->key, key)) {
                free_val_fnc(entry->entry);
                entry->entry = val;
                return;
            }
        }

        /* put this entry at the start of the bucket */
        entry = H_entry_alloc(key, val);
        entry->next = H->slots[i];
    }

    H->slots[i] = entry;

    ++(H->num_used_slots);

    if(D_LOAD_FACTOR <= ((float)H->num_used_slots / (float)H->num_slots)) {
        H->grow_table = 1;
    }
}

/**
 * Delete a record from a hash table.
 */
void dict_unset(H_type *H,
                H_key_type key,
                H_free_key_fnc_type free_key_fnc,
                H_free_val_fnc_type free_val_fnc) {

    H_Entry *entry = NULL,
            *prev = NULL,
            *next = NULL;

    uint32_t i;

    assert_not_null(H);
    assert_not_null(free_key_fnc);
    assert_not_null(free_val_fnc);

    i = H->key_hash_fnc(key) % H->num_slots;
    entry = H->slots[i];

    while(is_not_null(entry)) {
        next = entry->next;

        if(!H->collision_fnc(entry->key, key)) {
            if(is_not_null(prev)) {
                prev->next = next;
            } else {
                H->slots[i] = next;
            }

            free_key_fnc(entry->key);
            free_val_fnc(entry->entry);
            dict_mem_free(entry);

            --(H->num_used_slots);

            break;
        }

        prev = entry;
        entry = next;
    }
    return;
}

/**
 * Get a record from a hash table.
 */
H_val_type dict_get(H_type *H, H_key_type key) {
    H_Entry *entry = H_entry_get(H, key);
    return (is_null(entry) ? NULL : entry->entry);
}

/**
 * Check if a record exists in a hash table.
 */
char dict_is_set(H_type *H, H_key_type key) {
    return is_not_null(H_entry_get(H, key));
}

/**
 * Return the number of used slots.
 */
uint32_t dict_size(H_type *H) {
    assert_not_null(H);
    return H->num_used_slots;
}

/* -------------------------------------------------------------------------- */

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
    return (a != b);
}
