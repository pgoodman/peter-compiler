/*
 * set.c
 *
 *  Created on: Jun 26, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-set.h>

#define S_DEFAULT_SIZE 8

static const unsigned int STOP_GROW_SIZE = (((unsigned int) -1) / 2);

/**
 * Allocate a new set on the heap with space for num_slots elements. If do_fill
 * is set then the slots are filled 8 at a time with 'fill'.
 */
static PSet *S_alloc(unsigned int num_slots, int do_fill, unsigned char fill) {
    PSet *set;
    uint32_t *map;

    /* make sure we will be able to address all of the slots we are
     * requesting */
    if(num_slots >= ((unsigned int) -1) >> 5) {
        mem_error("Unable to allocate new set on the heap.");
    }

    set = mem_calloc(1, sizeof(PSet));
    map = mem_alloc(num_slots * sizeof(uint32_t));

    if(do_fill) {
        memset(map, fill, num_slots * 4);
    }

    if(is_null(set) || is_null(map)) {
        std_error("Unable to allocate new set on the heap.");
    }

    set->num_bits = num_slots * 32;
    set->num_slots = num_slots;
    set->num_entries = 0;
    set->map = map;

    return set;
}

/**
 * Grow a set by factors of two until it has reached a ceiling size.
 */
static void S_grow(PSet *set, unsigned int elm, unsigned char fill) {
    unsigned int size = set->num_bits;
    uint32_t *elms = set->map;

    while(size < elm) {
        if(size >= STOP_GROW_SIZE) {
            size = elm;
            break;
        }
        size *= 2;
    }

    elms = mem_realloc(elms, sizeof(uint32_t) * size);
    if(is_null(elms)) {
        mem_error("Unable to resize set.");
    }

    /* clear out all of the new elements */
    memset(
       elms + set->num_slots,
       fill,
       (size - set->num_slots) * (sizeof(uint32_t) / sizeof(char))
    );

    set->map = elms;
    set->num_slots = size;
    set->num_bits = size * 32;
}

/**
 * Toggle a bit in the set.
 */
static void S_set(PSet *set, unsigned int elm, unsigned int toggle) {
    unsigned int bit = 32 - (elm % 32),
                 i = (elm / 32);
    set->map[i] = (toggle ? set->map[i] | 1 << bit : set->map[i] & ~(1 << bit));
}

/**
 * Return the number of bits that are set in a 32-bit block.
 */
static unsigned int S_num_bits(uint32_t slot) {
    unsigned int bit_count = slot
                           - ((slot >> 1) & 033333333333)
                           - ((slot >> 2) & 011111111111);
    return ((bit_count + (bit_count >> 3)) & 030707070707) % 63;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a new set.
 */
PSet *set_alloc(void) {
    return S_alloc(S_DEFAULT_SIZE, 1, 0);
}

/**
 * Allocate a new set of everything (of the default size).
 */
PSet *set_alloc_inverted(void) {
    PSet *set = S_alloc(S_DEFAULT_SIZE, 1, (unsigned char) -1);
    set->num_entries = set->num_slots;
    return set;
}

/**
 * Free a set.
 */
void set_free(PSet *set) {
    assert_not_null(set);
    mem_free(set->map);
    mem_free(set);
}

/**
 * Add an element to a set.
 */
void set_add_elm(PSet *set, unsigned int elm) {
    assert_not_null(set);
    if(elm > set->num_bits) {
        S_grow(set, elm, 0);
    }
    S_set(set, elm, 1);
}

/**
 * Remove an element from a set.
 */
void set_remove_elm(PSet *set, unsigned int elm) {
    assert_not_null(set);
    if(set_has_elm(set, elm)) {
        --(set->num_entries);
        S_set(set, elm, 0);
    }
}

/**
 * Check if the elm is a member of the set.
 */
int set_has_elm(PSet *set, unsigned int elm) {
    assert_not_null(set);
    if(elm >= set->num_bits) {
        return 0;
    }
    return set->map[(unsigned int) (elm / 32)] & (1 << (32 - (elm % 32)));
}

/**
 * Check if possible_subset is a subset of super_set.
 */
int set_is_subset(PSet *super_set, PSet *possible_subset) {

    unsigned int max;
    uint32_t *super,
             *sub;

    assert_not_null(super_set);
    assert_not_null(possible_subset);

    if(possible_subset->num_slots > super_set->num_slots) {
        return 0;
    } else if(possible_subset->num_entries == 0) {
        return 1;
    }

    max = possible_subset->num_slots;
    super = super_set->map;
    sub = possible_subset->map;

    for(; max--; ++super, ++sub) {
        if((*sub & *super) != *sub) {
            return 0;
        }
    }

    return 1;
}

/**
 * Return a new set that is the intersection of set_a and set_b, i.e. it
 * contains only those elements common to both sets.
 */
PSet *set_intersect(PSet *set_a, PSet *set_b) {
    PSet *new_set;
    unsigned int new_num_slots,
                 new_num_entries = 0;
    uint32_t *elm_a,
             *elm_b,
             *elm_new;

    assert_not_null(set_a);
    assert_not_null(set_b);

    new_num_slots = (set_a->num_slots > set_b->num_slots)
                  ? set_a->num_slots : set_b->num_slots;

    new_set = S_alloc(new_num_slots, 0, 0);
    elm_new = new_set->map;
    elm_a = set_a->map;
    elm_b = set_b->map;

    for(; new_num_slots--; ++elm_a, ++elm_b, ++elm_new) {
        *elm_new = *elm_a & *elm_b;
        new_num_entries += S_num_bits(*elm_new);
    }

    new_set->num_entries = new_num_entries;

    return new_set;
}

/**
 * Return a new set that is the union of both sets. The new set will be no
 * bigger than max{|set_a|, |set_b|}.
 */
PSet *set_union(PSet *set_a, PSet *set_b) {

    PSet *new_set;

    unsigned int prefix_size,
                 suffix_size,
                 new_num_entries = 0,
                 new_num_slots;

    uint32_t *elm_large,
             *elm_small,
             *elm_new;

    assert_not_null(set_a);
    assert_not_null(set_b);

    if(set_a->num_slots > set_b->num_slots) {
        elm_large = set_a->map;
        elm_small = set_b->map;
        new_num_slots = set_a->num_slots;
        prefix_size = set_b->num_slots;
        suffix_size = new_num_slots - prefix_size;
    } else {
        elm_large = set_b->map;
        elm_small = set_a->map;
        new_num_slots = set_b->num_slots;
        prefix_size = set_a->num_slots;
        suffix_size = new_num_slots - prefix_size;
    }

    new_set = S_alloc(new_num_slots, 0, 0);
    elm_new = new_set->map;

    for(; prefix_size--; ++elm_large, ++elm_small, ++elm_new) {
        *elm_new = *elm_large | *elm_small;
        new_num_entries += S_num_bits(*elm_new);
    }

    for(; suffix_size--; ++elm_large, ++elm_new) {
        *elm_new = *elm_large;
        new_num_entries += S_num_bits(*elm_new);
    }

    return new_set;
}
