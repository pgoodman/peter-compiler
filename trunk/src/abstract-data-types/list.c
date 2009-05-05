/*
 * list.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "delegate.h"

/**
 * Allocate a new linked list.
 */
void *list_alloc(int size) {
    if(size < sizeof(List))
        size = sizeof(List);

    void *L = mem_alloc(size);

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

    if(NULL == free_list)
       free_list = &free;

    while(NULL != L) {
        next = L->next;
        free_list(curr);
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

    if(NULL == free_elm)
        free_elm = &free;

    while(NULL != L) {
        next = L->next;
        free_elm(L->elm);
        free(L);
        L = next;
    }
}
