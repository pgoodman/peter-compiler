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

typedef struct Vector {
    uint32_t _num_slots,
             _num_used_slots;
    void **_elms;
} Vector;

typedef struct VectorGenerator {
    Generator _;
    Vector *vec;
    uint32_t pos;
} VectorGenerator;

void *gen_vector_alloc(size_t, const uint32_t $$);
Vector *vector_alloc(const uint32_t $$);
void vector_free(Vector *, D1_t $$);
uint32_t vector_num_slots(Vector * $$);
uint32_t vector_num_used_slots(Vector * $$);
void vector_set(Vector *, uint32_t, void *, D1_t $$);
void vector_unset(Vector *, uint32_t, D1_t $$);
void *vector_get(Vector *, uint32_t $$);

VectorGenerator *vector_generator_alloc(Vector * $$);

#endif /* VECTOR_H_ */
