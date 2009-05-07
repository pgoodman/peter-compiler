/*
 * generator.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef GENERATOR_H_
#define GENERATOR_H_

#include <stdlib.h>
#include <memory-management/mem.h>
#include "delegate.h"
#include "function.h"

typedef struct Generator {
    F1 _gen;
    D1 _free;
    void *_curr;
} Generator;

void *generator_alloc(int);
void generator_free(void *);
void generator_init(void *, F1, D1);
int generator_next(void *);
void *generator_current(void *);

#endif /* GENERATOR_H_ */
