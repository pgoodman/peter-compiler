/*
 * list.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <stdio.h>
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

    if(NULL == free_list)
       free_list = &mem_free;

    while(NULL != L) {
        next = L->next;

#ifdef MEM_DEBUG
        if((void *)free_list == (void *)&mem_free)
            mem_free(L MEM_DEBUG_INFO);
        else
#endif
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

    if(NULL == free_elm)
        free_elm = &mem_free;
/*
    int i = 0;
    GenericList *list = L;
    while(NULL != list) {
        printf("%d,",i++);
        list = (GenericList *) ((List *) list)->next;
    }
*/
    printf("\nstarting to free list...\n");
    while(NULL != L) {
        next = (GenericList *) (((List *) L)->next);

#ifdef MEM_DEBUG
        if((void *)free_elm == (void *)&mem_free)
            mem_free(L->elm MEM_DEBUG_INFO);
        else
#endif
        free_elm(L->elm);

        L->elm = NULL;
        mem_free(L MEM_DEBUG_INFO);
        L = next;
    }
    printf("list freed.\n");
    fflush(stdout);
}
