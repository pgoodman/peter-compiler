/*
 * generator.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-generator.h>

/**
 * Allocate a new generator on the heap.
 */
void *generator_alloc(size_t size $$) { $H
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

    return_with G;
}

/**
 * Free the memory allocated by the generator.
 */
void generator_free(void *Cg $$) { $H
	assert(NULL != Cg);
    ((Generator *) Cg)->_free(Cg _$$);
    return_with;
}

/**
 * Initialize the generator.
 */
void generator_init(void *gen, F1_t gen_next, D1_t gen_free_fnc $$) { $H
    Generator *G;

	assert(NULL != gen);

    G = (Generator *)gen;
    G->_gen = gen_next;
    G->_free = gen_free_fnc;

    return_with;
}

/**
 * Advance to the next node in the generator.
 */
char generator_next(void *Cg $$) { $H
    Generator *G = NULL;

	assert(NULL != Cg);

    G = (Generator *) Cg;
    G->_curr = G->_gen(Cg _$$);

    return_with (NULL != G->_curr);
}

/**
 * Get the current node in the generator.
 */
void *generator_current(void *Cg $$) { $H
	assert(NULL != Cg);
    return_with ((Generator *) Cg)->_curr;
}
