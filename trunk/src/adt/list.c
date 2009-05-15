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
    while(is_not_null(L)) {
        next = ((PList *) L)->_next;
        free_list_fnc(L );
        L = next;
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
    while(is_not_null(L)) {
        next = (PGenericList *) list_get_next(L);
        free_elm_fnc(L->_elm );
        L->_elm = NULL;
        mem_free(L);
        L = next;
    }

    return;
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
