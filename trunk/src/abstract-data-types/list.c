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
void *list_alloc(int struct_size) {
    if(struct_size < sizeof(List))
        struct_size = sizeof(List);

    void *L = mem_alloc(struct_size MEM_DEBUG_INFO);

    if(NULL == L)
        mem_error("Unable to allocate a linked list on the heap.");

    ((List *) L)->_next = NULL;

    return L;
}

/**
 * Free a list.
 */
void list_free(void *L, D1 free_list_fnc) {
    List *next = NULL;

    if(NULL == free_list_fnc || (void *)free_list_fnc == (void *)&mem_free)
        free_list_fnc = &mem_free_no_debug;

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
    return NULL != L ? ((List *) L)->_next : NULL;
}

/**
 * Allocate a generic list on the heap.
 */
inline GenericList *gen_list_alloc(void) {
    GenericList *L = list_alloc(sizeof(GenericList));
    L->_elm = NULL;
    return L;
}

/**
 * Free the allocated space of a generic list.
 */
void gen_list_free(GenericList *L, D1 free_elm_fnc) {
    GenericList *next = NULL;

    if(NULL == free_elm_fnc || (void *)free_elm_fnc == (void *)&mem_free)
        free_elm_fnc = &mem_free_no_debug;

    while(NULL != L) {
        next = (GenericList *) list_get_next(L);
        free_elm_fnc(L->_elm);
        L->_elm = NULL;
        mem_free(L MEM_DEBUG_INFO);
        L = next;
    }
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
