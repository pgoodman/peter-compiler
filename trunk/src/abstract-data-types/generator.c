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
    void *G = NULL;
    Generator *g = NULL;

    if(size < sizeof(Generator))
        size = sizeof(Generator);

    G = mem_alloc(size MEM_DEBUG_INFO);
    if(NULL == G)
        mem_error("Unable to allocate generator on the heap.");

    // set the default values for these things
    g = (Generator *) G;
    g->_free = NULL;
    g->_gen = NULL;
    g->_curr = NULL;

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
 * Advance to the next node in the generator.
 */
int generator_next(void *Cg) {
    Generator *G = NULL;

    if(NULL == Cg)
        return 0;

    G = (Generator *) Cg;
    G->_curr = G->_gen(Cg);

    return NULL != G->_curr;
}

/**
 * Get the current node in the generator.
 */
void *generator_current(void *Cg) {
    if(NULL == Cg)
        return NULL;

    return ((Generator *) Cg)->_curr;
}
