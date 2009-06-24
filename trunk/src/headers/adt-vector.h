/*
 * vector.h
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef VECTOR_H_
#define VECTOR_H_

#include "std-include.h"
#include "func-delegate.h"
#include "adt-generator.h"

typedef struct PVector {
    uint32_t _num_slots,
             _num_used_slots;
    void **_elms;
} PVector;

typedef struct PVectorGenerator {
    PGenerator _;
    PVector *vec;
    uint32_t pos;
} PVectorGenerator;

void *gen_vector_alloc(const size_t, const uint32_t );
PVector *vector_alloc(const uint32_t );
void vector_free(PVector *, PDelegate *);
uint32_t vector_num_slots(PVector * );
uint32_t vector_num_used_slots(PVector * );
void vector_set(PVector *, uint32_t, void *, PDelegate *);
void vector_unset(PVector *, uint32_t, PDelegate *);
void *vector_get(PVector *, uint32_t );

PVectorGenerator *vector_generator_alloc(PVector * );

#endif /* VECTOR_H_ */
