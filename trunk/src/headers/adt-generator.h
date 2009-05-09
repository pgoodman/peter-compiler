/*
 * generator.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef GENERATOR_H_
#define GENERATOR_H_

#include "std-include.h"
#include "func-delegate.h"
#include "func-function.h"

typedef struct Generator {
    F1_t _gen;
    D1_t _free;
    void *_curr;
} Generator;

void *generator_alloc(size_t $$);
void generator_free(void * $$);
void generator_init(void *, F1_t, D1_t $$);
char generator_next(void * $$);
void *generator_current(void * $$);

#endif /* GENERATOR_H_ */
