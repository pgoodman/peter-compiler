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
void *generator_alloc(size_t size) {
    void *G = NULL;
    Generator *g = NULL;

    if(size < sizeof(Generator))
        size = sizeof(Generator);

    G = mem_alloc(size);
    if(NULL == G)
        mem_error("Unable to allocate generator on the heap.");

    /* set the default values for these things */
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
	assert(NULL != Cg);
    ((Generator *) Cg)->_free(Cg);
}

/**
 * Initialize the generator.
 */
void generator_init(void *gen, F1_t gen_next, D1_t gen_free_fnc) {
    Generator *G;

	assert(NULL != gen);

    G = (Generator *)gen;
    G->_gen = gen_next;
    G->_free = gen_free_fnc;
}

/**
 * Advance to the next node in the generator.
 */
char generator_next(void *Cg) {
    Generator *G = NULL;

	assert(NULL != Cg);

    G = (Generator *) Cg;
    G->_curr = G->_gen(Cg);

    return NULL != G->_curr;
}

/**
 * Get the current node in the generator.
 */
void *generator_current(void *Cg) {
	assert(NULL != Cg);
    return ((Generator *) Cg)->_curr;
}
