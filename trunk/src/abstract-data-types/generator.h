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
#include "mem.h"
#include "delegate.h"
#include "function.h"

typedef struct Generator {
    F1 _gen;
    D1 _free;
} Generator;

void *generator_alloc(int);
void generator_free(void *);
void *generator_next(void *);

#endif /* GENERATOR_H_ */
