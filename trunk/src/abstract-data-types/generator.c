/*
 * generator.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <stdlib.h>
#include "mem.h"
#include "generator.h"


/**
 * Allocate a new generator on the heap.
 */
void *generator_alloc(int size) {
    void *G = NULL;
    Generator *g = NULL;

    if(size < sizeof(Generator))
        size = sizeof(Generator);

    G = mem_alloc(size);
    if(NULL == G)
        mem_error("Unable to allocate generator on the heap.");

    // set the default values for these things
    g = (Generator *) G;
    g->_free = NULL;
    g->_gen = NULL;

    return G;
}

/**
 * Free the memory allocated by the generator.
 */
void generator_free(void *Cg) {
    if(NULL == Cg)
        return;

    ((Generator *) Cg)->_free(Cg);
}

/**
 * Return the next node in the generator or null otherwise.
 */
void *generator_next(void *Cg) {
    if(NULL == Cg)
        return NULL;
    return ((Generator *) Cg)->_gen(Cg);
}
