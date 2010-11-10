/*
 * vector.c
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-vector.h>

static const uint32_t STOP_GROW_SIZE = (((uint32_t) -1) / 2);

/**
 * Allocate the slots used by a vector.
 */
static void **V_alloc_slots(uint32_t num_slots) {
    void **slots = mem_calloc(num_slots, sizeof(void *));

    if(is_null(slots)) {
        mem_error("Unable to allocate vector slots.");
    }
    return slots;
}

/**
 * Resize the vector so that it has at least i slots in it.
 */
static void V_resize(PVector *V, uint32_t i) {
    uint32_t new_size;

    assert_not_null(V);

    /* don't perform any resize operation */
    if(i < V->_num_slots) {
        return;
    }

    /* grow the capacity without allowing integer to overflow */
    new_size = V->_num_slots;
    while(new_size < i) {
        if(new_size > STOP_GROW_SIZE) {
            new_size = i;
        }
        new_size *= 2;
    }

    if(new_size < i)
        new_size = 0xFFFFFFFF;

    V->_elms = mem_realloc(V->_elms, (new_size * sizeof(void *)));
    if(is_null(V->_elms)) {
        mem_error("Unable to resize the vector.");
    }

    V->_num_slots = new_size;
    return;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a generic vector on the heap.
 */
void *gen_vector_alloc(const size_t struct_size, const uint32_t num_slots) {
    void **elms = NULL,
         *vec = NULL;
    PVector *V = NULL;

    assert(sizeof(PVector) <= struct_size);

    vec = mem_alloc(struct_size);
    if(is_null(vec)) {
        mem_error("Unable to allocate vector on the heap.");
    }

    elms = V_alloc_slots(num_slots );

    /* initialize the vector */
    V = (PVector *) vec;
    V->_elms = elms;
    V->_num_slots = num_slots;
    V->_num_used_slots = 0;

    return vec;
}

/**
 * Allocate a vector on the heap.
 */
PVector *vector_alloc(const uint32_t num_slots) {
    return (PVector *) gen_vector_alloc(sizeof(PVector), num_slots);
}

/**
 * Free a vector and all of its elements.
 */
void vector_free(PVector *V, PDelegate *free_elm_fnc) {
    uint32_t i;

    assert_not_null(V);

	/* free up every non-null slot. */
    for(i = 0; i < V->_num_slots; ++i) {
        if(is_not_null(V->_elms[i])) {
            free_elm_fnc(V->_elms[i] );
        }
    }

    mem_free(V->_elms);

    V->_elms = NULL;

    mem_free(V);

    V = NULL;

    return;
}

/**
 * Return the size of a vector.
 */
uint32_t vector_num_slots(PVector *V) {
	assert_not_null(V);
    return V->_num_slots;
}

/**
 * Return the number of used slots in a vector.
 */
uint32_t vector_num_used_slots(PVector *V) {
	assert_not_null(V);
    return V->_num_used_slots;
}

/**
 * Set the value of position i in a vector. A free element function pointer
 * can also be passed in so that we can choose to free what the slot pointed
 * too if we are overwriting it.
 */
void vector_set(PVector *V, uint32_t i, void *elm, PDelegate *free_elm_fnc) {
    char slot_increment = 1;

	assert_not_null(V);
	assert_not_null(elm);

    /* are we overwriting an already used slot? */
    if(i <= V->_num_slots) {
        vector_unset(V, i, free_elm_fnc );

        if(is_not_null(V->_elms[i])) {
            slot_increment = 0;
        }

    /* resize the vector */
    } else {
        V_resize(V, i );
    }

    /* update the stuff :) */
    V->_elms[i] = elm;
    V->_num_used_slots += slot_increment;

    return;
}

/**
 * Unset the value at a position in a vector.
 */
void vector_unset(PVector *V, uint32_t i, PDelegate *free_elm_fnc) {
	assert_not_null(V);
	assert(i < V->_num_slots);

	if(is_not_null(V->_elms[i])) {
        V->_elms[i] = NULL;
        free_elm_fnc(V->_elms[i] );
	}

    return;
}

/**
 * Get an element from a slot in the vector.
 */
void *vector_get(PVector *V, uint32_t i) {
	assert_not_null(V);
	assert(i < V->_num_slots);

    return V->_elms[i];
}

/* -------------------------------------------------------------------------- */

/**
 * Get the next element in a vector.
 */
static void *V_generator_next(void *g) {
    PVectorGenerator *G = g;
    PVector *V = NULL;
    uint32_t i;

    assert_not_null(G);

    V = G->vec;
    i = G->pos;

	assert_not_null(V);

	/* ignore empty vectors and index-out-of range. we don't assert the range
	 * check because generator_next() expects this function to return null
	 * if no next element exist and because we take advantage of the fact that
	 * out of range doesn't exist to force null. */
    if(V->_num_used_slots == 0 || V->_num_slots < i) {
        return NULL;
    }

	/* loop through the vector until we find a non-null pointer. */
    for(i = G->pos; is_null(V->_elms[i]); ++i)
        ;

    ++(G->pos);

    return V->_elms[i];
}

/**
 * Free a vector generator.
 */
static void V_generator_free(void *g) {
    PVectorGenerator *G = g;

    assert_not_null(G);

    G->vec = NULL;
    mem_free(G);
    G = NULL;
    return;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a new vector generator on the heap. Note: we are allowed to have
 * a generator over a NULL vector.
 */
PVectorGenerator *vector_generator_alloc(PVector *V) {
    PVectorGenerator *G = NULL;

    assert_not_null(V);

    G = mem_alloc(sizeof(PVectorGenerator));
    if(is_null(G)) {
        mem_error("Unable to allocate vector generator on the heap.");
    }

    G->vec = V;
    G->pos = 0;

    generator_init(G, &V_generator_next, &V_generator_free);

    return G;
}
