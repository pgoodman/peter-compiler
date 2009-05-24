/*
 * list.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-list.h>

/**
 * Allocate a new linked list.
 */
void *list_alloc(const size_t struct_size ) {
    void *L = NULL;

    assert(sizeof(PList) <= struct_size);

    L = mem_alloc(struct_size);
    if(is_null(L)) {
        mem_error("Unable to allocate a linked list on the heap.");
    }

    ((PList *) L)->_next = NULL;

    return L;
}

/**
 * Free a list.
 */
void list_free(void *L, PDelegate free_list_fnc ) {
    PList *next = NULL;

	assert_not_null(free_list_fnc);

	/* Free up the list. */
    for(; is_not_null(L); L = next) {
        next = (PList *) list_get_next(L);
        free_list_fnc(L);
    }

    return;
}

/**
 * Set the next element in a linked list.
 */
void list_set_next(void *L, void *N ) {
    assert_not_null(L);
    ((PList *) L)->_next = (PList *) N;
    return;
}

/**
 * Get the next element of a linked list.
 */
PList *list_get_next(void *L) {
	assert_not_null(L);
    return ((PList *) L)->_next;
}

/**
 * Check if a list has a next element.
 */
char list_has_next(void *L) {
    assert_not_null(L);
    return (NULL != ((PList *) L)->_next);
}

/**
 * Allocate a generic list on the heap.
 */
PGenericList *gen_list_alloc(void) {
    PGenericList *L = list_alloc(sizeof(PGenericList));
    L->_elm = NULL;
    return L;
}

/**
 * Free the allocated space of a generic list.
 */
void gen_list_free(PGenericList *L, PDelegate free_elm_fnc ) {
    PGenericList *next = NULL;

    assert_not_null(free_elm_fnc);

	/* Go through the chain and free the lists and their respective
	 * elements. */
    for(; is_not_null(L); L = next) {
        next = (PGenericList *) list_get_next(L);
        free_elm_fnc(L->_elm);
        L->_elm = NULL;
        mem_free(L);
    }

    return;
}

/**
 * Allocate a chain of lists at one time.
 */
PGenericList *gen_list_alloc_chain(const unsigned int chain_length) {
    PGenericList *L,
                 *prev = NULL;
    unsigned int i;

    assert(chain_length >= 1);

    L = mem_alloc(sizeof(PGenericList) * chain_length);
    if(is_null(L)) {
        mem_error("Unable to allocate chain of generic lists on the heap.");
    }

    /* initialize the elements of the chain */
    prev = L;
    prev->_elm = NULL;
    ((PList *) prev)->_next = NULL;

    for(i = 1; i < chain_length; ++i) {
        ((PList *) prev)->_next = (L+i);
        ((PList *) (L+i))->_next = NULL;
        (L+i)->_elm = NULL;
        prev = L+i;
    }

    return L;
}

/**
 * Free a list that was allocated as a chain.
 */
void gen_list_free_chain(PGenericList *L, PDelegate free_elm_fnc) {
    PGenericList *curr;
    for(curr = L; is_not_null(curr); curr = (PGenericList *) list_get_next(curr)) {
        free_elm_fnc(curr->_elm);
    }
    mem_free(L);
}

/**
 * Free only the element of a single list.
 */
void gen_list_free_elm(PGenericList *L, PDelegate free_elm_fnc ) {
	assert_not_null(L);
    free_elm_fnc(L->_elm );
    return;
}

/**
 * Get the list element from a generic list.
 */
void *gen_list_get_elm(PGenericList *L ) {
    assert_not_null(L);
    return L->_elm;
}

/**
 * Set the list element from a generic list.
 */
void gen_list_set_elm(PGenericList *L, void *elm ) {
    assert_not_null(L);
    L->_elm = elm;
    return;
}
