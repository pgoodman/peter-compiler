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
void *list_alloc(int size) {
    if(size < sizeof(List))
        size = sizeof(List);

    void *L = mem_alloc(size MEM_DEBUG_INFO);

    if(NULL == L)
        mem_error("Unable to allocate a linked list on the heap.");

    ((List *) L)->next = NULL;

    return L;
}

/**
 * Free a list.
 */
void list_free(List *L, D1 free_list) {
    List *next;

    if(NULL == free_list || (void *)free_list == (void *)&mem_free)
        free_list = &mem_free_no_debug;

    while(NULL != L) {
        next = L->next;
        free_list(L);
        L = next;
    }
}

/**
 * Allocate a generic list on the heap.
 */
inline GenericList *gen_list_alloc(void) {
    GenericList *L = list_alloc(sizeof(GenericList));
    L->elm = NULL;
    return L;
}

/**
 * Free the allocated space of a generic list.
 */
void gen_list_free(GenericList *L, D1 free_elm) {
    GenericList *next;

    if(NULL == free_elm || (void *)free_elm == (void *)&mem_free)
        free_elm = &mem_free_no_debug;

    while(NULL != L) {
        next = (GenericList *) (((List *) L)->next);
        free_elm(L->elm);
        L->elm = NULL;
        mem_free(L MEM_DEBUG_INFO);
        L = next;
    }
}
