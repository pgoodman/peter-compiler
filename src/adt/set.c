/*
 * set.c
 *
 *  Created on: Jun 26, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-set.h>

#define S_DEFAULT_SIZE 4

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
static void S_grow(PSet *set, unsigned int elm, int do_fill, unsigned char fill) {
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
    if(do_fill) {
        memset(
           elms + set->num_slots,
           fill,
           (size - set->num_slots) * (sizeof(uint32_t) / sizeof(char))
        );
    }

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

/**
 * Perform the union of set_a and set_b into new_set. This function assumes
 * that new_set is at least as large as the larger of set_a and set_b.
 */
static void S_union(PSet *set_a, PSet *set_b, PSet *new_set) {

    uint32_t *elm_large,
             *elm_small,
             *elm_new;

    unsigned int prefix_size,
                 suffix_size,
                 new_num_entries = 0;

    assert_not_null(set_a);
    assert_not_null(set_b);

    if(set_a->num_slots > set_b->num_slots) {
        elm_large = set_a->map;
        elm_small = set_b->map;
        prefix_size = set_b->num_slots / 4;
        suffix_size = (set_a->num_slots / 4) - prefix_size;
    } else {
        elm_large = set_b->map;
        elm_small = set_a->map;
        prefix_size = set_a->num_slots / 4;
        suffix_size = (set_b->num_slots / 4) - prefix_size;
    }

    elm_new = new_set->map;

    for(; prefix_size--; ++elm_large, ++elm_small, ++elm_new) {
        *elm_new = *elm_large | *elm_small;
        new_num_entries += S_num_bits(*elm_new);
        *++elm_new = *++elm_large | *++elm_small;
        new_num_entries += S_num_bits(*elm_new);
        *++elm_new = *++elm_large | *++elm_small;
        new_num_entries += S_num_bits(*elm_new);
        *++elm_new = *++elm_large | *++elm_small;
        new_num_entries += S_num_bits(*elm_new);
    }

    for(; suffix_size--; ++elm_large, ++elm_new) {
        *elm_new = *elm_large | *elm_small;
        new_num_entries += S_num_bits(*elm_new);
        *++elm_new = *++elm_large | *++elm_small;
        new_num_entries += S_num_bits(*elm_new);
        *++elm_new = *++elm_large | *++elm_small;
        new_num_entries += S_num_bits(*elm_new);
        *++elm_new = *++elm_large | *++elm_small;
        new_num_entries += S_num_bits(*elm_new);
    }

    new_set->num_entries = new_num_entries;
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
    if(is_null(set)) {
        return;
    }
    mem_free(set->map);
    mem_free(set);
}

/**
 * Add an element to a set.
 */
void set_add_elm(PSet *set, unsigned int elm) {
    int had_elm = 0;

    assert_not_null(set);

    if(elm > set->num_bits) {
        S_grow(set, elm, 1, 0);
        ++(set->num_entries);
    } else if(!set_has_elm(set, elm)) {
        ++(set->num_entries);
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
 * Truncate a set down to the default size and then remove all elements.
 */
void set_truncate(PSet *set) {
    if(is_not_null(set) && set->num_slots > S_DEFAULT_SIZE) {
        set->num_slots = S_DEFAULT_SIZE;
        set->num_bits = S_DEFAULT_SIZE * 32;
        set->map = mem_realloc(set->map, sizeof(uint32_t) * S_DEFAULT_SIZE);
        if(is_null(set->map)) {
            mem_error("Internal Set Error: Unable to truncate set.");
        }
    }

    set_empty(set);
}

/**
 * Remove all elements from a set.
 */
void set_empty(PSet *set) {
    int max;
    uint32_t *map;

    /* try to either fail or succeed fast */
    if(is_null(set) || !set->num_entries) {
        return;
    }

    max = set->num_slots / 4;
    map = set->map;

    /* go eight cells at a time and compare the sets */
    for(; --max >= 0; ) {
        *map = 0;
        *++map = 0;
        *++map = 0;
        *++map = 0;
    }
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
 * Check if has the same contents and size as another set. Both sets also need
 * to have the same number of allocated slots, which is a stronger requirement
 * than necessary as a set can have a large item put into it and then removed
 * and still be the same as the smaller set, and so these two sets would not
 * be seen as equal by this algorithm.
 */
int set_equals(const PSet *set_a, const PSet *set_b) {
    int max;

    uint32_t *map_a,
             *map_b;

    /* try to either fail or succeed fast */
    if(set_a == set_b) {
        return 1;
    } else if(is_null(set_a) || is_null(set_b)) {
        return 0;
    } else if(set_a->num_entries != set_b->num_entries
           || set_a->num_slots != set_b->num_slots) {
        return 0;
    }

    max = set_a->num_slots / 4;
    map_a = set_a->map;
    map_b = set_b->map;

    /* go eight cells at a time and compare the sets */
    for(; --max >= 0; ) {
        if(*map_a != *map_b
        || *++map_a != *++map_b
        || *++map_a != *++map_b
        || *++map_a != *++map_b) {
            return 0;
        }
    }

    return 1;
}

/**
 * Check if two sets are not equivalent.
 */
int set_not_equals(PSet *set_a, PSet *set_b) {
    return !set_equals(set_a, set_b);
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

    assert_not_null(set_a);
    assert_not_null(set_b);

    if(set_a->num_slots > set_b->num_slots) {
        new_set = S_alloc(set_a->num_slots, 0, 0);
    } else {
        new_set = S_alloc(set_b->num_slots, 0, 0);
    }

    S_union(set_a, set_b, new_set);

    return new_set;
}

/**
 * Perform an in-place union of set_a and set_b into set_a.
 */
void set_union_inplace(PSet *set_a, PSet *set_b) {

    PSet *new_set;

    assert_not_null(set_a);
    assert_not_null(set_b);

    if(set_a->num_slots < set_b->num_slots) {
        S_grow(set_a, set_b->num_bits, 0, 0);
    }

    S_union(set_a, set_b, set_a);
}

/**
 * Map a function over the elements of a set.
 */
void set_map(PSet *set, void *state, PSetMap *map_fnc) {
    unsigned int i, j;
    uint32_t *map;

    assert_not_null(set);

    map = set->map;
    j = set->num_bits;

    for(i = 0; i < j; ++i) {
        if(map[(unsigned int) (i / 32)] & (1 << (32 - (i % 32)))) {
            map_fnc(state, i);
        }
    }
}

/**
 * Return the number of elements in the set.
 */
unsigned int set_cardinality(const PSet *set) {
    if(is_null(set)) {
        return 0;
    }
    return set->num_entries;
}

/**
 * Return the largest element in the set, or -1 on failure.
 */
int set_max_elm(const PSet *set) {
    uint32_t *map;
    int j;

    assert_not_null(set);

    map = set->map;
    j = set->num_bits;

    for(; j--; ) {
        if(map[(unsigned int) (j / 32)] & (1 << (32 - (j % 32)))) {
            return j;
        }
    }

    return -1;
}

/**
 * Hash the contents of a set using murmurhash.
 */
uint32_t set_hash(PSet *set) {
    assert_not_null(set);
    return murmur_hash((char *) set->map, set->num_slots * 4, 73);
}
