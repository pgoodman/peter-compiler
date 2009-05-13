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
void *generator_alloc(const size_t size ) { $H
    void *G = NULL;
    PGenerator *g = NULL;

    assert(sizeof(PGenerator) <= size);

    G = mem_alloc(size);
    if(NULL == G) {
        mem_error("Unable to allocate generator on the heap.");
    }

    /* set the default values for these things */
    g = (PGenerator *) G;
    g->_free = NULL;
    g->_gen = NULL;
    g->_curr = NULL;

    return_with G;
}

/**
 * Free the memory allocated by the generator.
 */
void generator_free(void *Cg ) { $H
	assert_not_null(Cg);

    ((PGenerator *) Cg)->_free(Cg);
    return_with;
}

/**
 * Initialize the generator.
 */
void generator_init(void *gen, PFunction gen_next, PDelegate gen_free_fnc ) { $H
	assert_not_null(gen);
    PGenerator *G;

    G = (PGenerator *) gen;
    G->_gen = gen_next;
    G->_free = gen_free_fnc;

    return_with;
}

/**
 * Advance to the next node in the generator.
 */
char generator_next(void *Cg ) { $H
	assert_not_null(Cg);

    PGenerator *G = NULL;

    G = (PGenerator *) Cg;
    G->_curr = G->_gen(Cg );

    return_with (NULL != G->_curr);
}

/**
 * Get the current node in the generator.
 */
void *generator_current(void *Cg ) { $H
	assert_not_null(Cg);
    return_with ((PGenerator *) Cg)->_curr;
}
