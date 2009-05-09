/*
 * vector.c
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "vector.h"

/**
 * Allocate the slots used by a vector.
 */
static void **V_alloc_slots(uint32_t num_slots) {
    void **slots = mem_calloc(num_slots, sizeof(void *));
    if(NULL == slots)
        mem_error("Unable to allocate vector slots.");
    return slots;
}

/**
 * Resize the vector so that it has at least i slots in it.
 */
static void V_resize(Vector *V, uint32_t i) {
    void **slots;
    uint32_t new_size,
             j,
             max_size = 0x7FFFFFFF;

    assert(NULL != V);

    /* don't perform any resize operation */
    if(i < V->_num_slots)
        return;

    /* grow the capacity without allowing integer to overflow */
    for(new_size = V->_num_slots; new_size < i && new_size <= max_size; new_size = new_size * 2)
        ;

    if(new_size < i)
        new_size = 0xFFFFFFFF;

    slots = V_alloc_slots(new_size);

    /* add in the old slots */
    for(j = 0; j < V->_num_slots; ++j)
        slots[j] = V->_elms[j];

    /* free the old memory and update our vector */
    mem_free(V->_elms);

    V->_elms = slots;
    V->_num_slots = new_size;
}

/**
 * Allocate a generic vector on the heap.
 */
void *gen_vector_alloc(size_t size, const uint32_t num_slots) {
    void **elms,
         *vec;
    Vector *V;

    if(size < sizeof(Vector))
        size = sizeof(Vector);

    vec = mem_alloc(size);
    if(NULL == vec)
        mem_error("Unable to allocate vector on the heap.");

    elms = V_alloc_slots(num_slots);

    /* initialize the vector */
    V = (Vector *) vec;
    V->_elms = elms;
    V->_num_slots = num_slots;
    V->_num_used_slots = 0;

    return vec;
}

/**
 * Allocate a vector on the heap.
 */
Vector *vector_alloc(const uint32_t num_slots) {
    return (Vector *) gen_vector_alloc(sizeof(Vector), num_slots);
}

/**
 * Free a vector and all of its elements.
 */
void vector_free(Vector *V, D1_t free_elm_fnc) {
    uint32_t i;

    assert(NULL != V);

	/* free up every non-null slot. */
    for(i = 0; i < V->_num_slots; ++i) {
        if(NULL != V->_elms[i])
            free_elm_fnc(V->_elms[i]);
    }

    mem_free(V->_elms);
    mem_free(V);

    V->_elms = NULL;
    V = NULL;
}

/**
 * Return the size of a vector.
 */
uint32_t vector_num_slots(Vector *V) {
	assert(NULL != V);
    return V->_num_slots;
}

/**
 * Return the number of used slots in a vector.
 */
uint32_t vector_num_used_slots(Vector *V) {
	assert(NULL != V);
    return V->_num_used_slots;
}

/**
 * Set the value of position i in a vector. A free element function pointer
 * can also be passed in so that we can choose to free what the slot pointed
 * too if we are overwriting it.
 */
void vector_set(Vector *V, uint32_t i, void *elm, D1_t free_elm_fnc) {

    char slot_increment = 1;

    assert(NULL != V && NULL != elm);

    /* are we overwriting an already used slot? */
    if(i <= V->_num_slots) {
        vector_unset(V, i, free_elm_fnc);

        if(NULL != V->_elms[i])
            slot_increment = 0;

    /* resize the vector */
    } else {
        V_resize(V, i);
    }

    /* update the stuff :) */
    V->_elms[i] = elm;
    V->_num_used_slots += slot_increment;
}

/**
 * Unset the value at a position in a vector.
 */
void vector_unset(Vector *V, uint32_t i, D1_t free_elm_fnc) {
	assert(NULL != V && i < V->_num_slots);

	if(NULL == V->_elms[i])
        return;

    V->_elms[i] = NULL;
    free_elm_fnc(V->_elms[i]);
}

/**
 * Get an element from a slot in the vector.
 */
void *vector_get(Vector *V, uint32_t i) {
    assert(NULL != V && i < V->_num_slots);
    return V->_elms[i];
}

/**
 * Get the next element in a vector.
 */
static void *V_generator_next(void *g) {
    VectorGenerator *G = g;
    Vector *V;
    uint32_t i;

    assert(NULL != G);

    V = G->vec;
    i = G->pos;

	assert(NULL != V);

	/* ignore empty vectors and index-out-of range. we don't assert the range
	 * check because generator_next() expects this function to return null
	 * if no next element exist and because we take advantage of the fact that
	 * out of range doesn't exist to force null. */
    if(V->_num_used_slots == 0 || V->_num_slots < i)
        return NULL;

	/* loop through the vector until we find a non-null pointer. */
    for(i = G->pos; NULL == V->_elms[i]; ++i)
        ;

    ++(G->pos);

    return V->_elms[i];
}

/**
 * Free a vector generator.
 */
static void V_generator_free(void *g) {
	assert(NULL != g);

    VectorGenerator *G = g;
    G->vec = NULL;

    mem_free(G);

    G = NULL;
}

/**
 * Allocate a new vector generator on the heap. Note: we are allowed to have
 * a generator over a NULL vector.
 */
VectorGenerator *vector_generator_alloc(Vector *V) {
    VectorGenerator *G = mem_alloc(sizeof(VectorGenerator));

    if(NULL == G)
        mem_error("Unable to allocate vector generator on the heap.");

    G->vec = V;
    G->pos = 0;

    generator_init(G, &V_generator_next, &V_generator_free);

    return G;
}
