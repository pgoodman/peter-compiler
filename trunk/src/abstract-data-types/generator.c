/*
 * generator.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "generator.h"

/**
 * Allocate a new generator on the heap.
 */
void *generator_alloc(int size) {
    void *G;

    if(size < sizeof(Generator))
        size = sizeof(Generator);

    G = malloc(size);
    if(NULL == G)
        mem_error("Unable to allocate generator on the heap.");

    return G;
}

void generator_free(void *Cg) {
    Generator *G = (Generator *)Cg;
}

/**
 *
 */
void *generator_next(void *Cg) {
    Generator *G = (Generator *)Cg;

}
