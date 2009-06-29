/*
 * adt-set.h
 *
 *  Created on: Jun 26, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef ADTSET_H_
#define ADTSET_H_

#include <string.h>

#include "std-include.h"
#include "vendor-murmur-hash.h"

typedef struct PSet {
    unsigned int num_bits,
                 num_slots,
                 num_entries;
    uint32_t *map;
} PSet;

typedef void (PSetMap)(void *state, unsigned int elm);

PSet *set_alloc(void);

void set_free(PSet *set);

PSet *set_alloc_inverted(void);

void set_add_elm(PSet *set, unsigned int elm);

void set_remove_elm(PSet *set, unsigned int elm);

int set_has_elm(PSet *set, unsigned int elm);

int set_is_subset(PSet *super_set, PSet *possible_subset);

int set_equals(const PSet *set_a, const PSet *set_b);

int set_not_equals(PSet *set_a, PSet *set_b);

PSet *set_intersect(PSet *set_a, PSet *set_b);

PSet *set_union(PSet *set_a, PSet *set_b);

void set_union_inplace(PSet *set_a, PSet *set_b);

void set_map(PSet *set, void *state, PSetMap *map_fnc);

unsigned int set_cardinality(const PSet *set);

int set_max_elm(const PSet *set);

uint32_t set_hash(PSet *set);

#endif /* ADTSET_H_ */
