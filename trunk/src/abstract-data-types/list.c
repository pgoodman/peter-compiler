/*
 * list.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "list.h"

/**
 * Allocate a new linked list.
 */
void *list_alloc(size_t struct_size) {
    if(struct_size < sizeof(List))
        struct_size = sizeof(List);

    void *L = mem_alloc(struct_size);

    if(NULL == L)
        mem_error("Unable to allocate a linked list on the heap.");

    ((List *) L)->_next = NULL;

    return L;
}

/**
 * Free a list.
 */
void list_free(void *L, D1_t free_list_fnc) {
    List *next = NULL;
	
	assert(NULL != free_list_fnc);

	/* Free up the list. */
    while(NULL != L) {
        next = ((List *) L)->_next;
        free_list_fnc(L);
        L = next;
    }
}

/**
 * Set the next element in a linked list.
 */
void list_set_next(void *L, void *N) {
    ((List *) L)->_next = (List *) N;
}

/**
 * Get the next element of a linked list.
 */
List *list_get_next(void *L) {
	assert(NULL != L);
	
    return ((List *) L)->_next;
}

/**
 * Allocate a generic list on the heap.
 */
GenericList *gen_list_alloc(void) {
    GenericList *L = list_alloc(sizeof(GenericList));
    L->_elm = NULL;
    return L;
}

/**
 * Free the allocated space of a generic list.
 */
void gen_list_free(GenericList *L, D1_t free_elm_fnc) {
    GenericList *next = NULL;

    assert(NULL != free_elm_fnc);

	/* Go through the chain and free the lists and their respective
	 * elements. */
    while(NULL != L) {
        next = (GenericList *) list_get_next(L);
        free_elm_fnc(L->_elm);
        L->_elm = NULL;
        mem_free(L);
        L = next;
    }
}

/**
 * Free only the element of a single list.
 */
void gen_list_free_elm(GenericList *L, D1_t free_elm_fnc) {
	assert(NULL != L);
    free_elm_fnc(L->_elm);
}

/**
 * Get the list element from a generic list.
 */
void *gen_list_get_elm(GenericList *L) {
    return L->_elm;
}

/**
 * Set the list element from a generic list.
 */
void gen_list_set_elm(GenericList *L, void *elm) {
    L->_elm = elm;
}
