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

typedef struct PSet {
    unsigned int num_bits,
                 num_slots,
                 num_entries;
    uint32_t *map;
} PSet;

PSet *set_alloc(void);

void set_free(PSet *set);

PSet *set_alloc_inverted(void);

void set_add_elm(PSet *set, unsigned int elm);

void set_remove_elm(PSet *set, unsigned int elm);

int set_has_elm(PSet *set, unsigned int elm);

int set_is_subset(PSet *super_set, PSet *possible_subset);

PSet *set_intersect(PSet *set_a, PSet *set_b);

PSet *set_union(PSet *set_a, PSet *set_b);

#endif /* ADTSET_H_ */
