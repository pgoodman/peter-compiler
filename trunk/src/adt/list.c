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
void *list_alloc(size_t struct_size $$) { $H
    if(struct_size < sizeof(List))
        struct_size = sizeof(List);

    void *L = mem_alloc(struct_size);

    if(NULL == L)
        mem_error("Unable to allocate a linked list on the heap.");

    ((List *) L)->_next = NULL;

    return_with L;
}

/**
 * Free a list.
 */
void list_free(void *L, D1_t free_list_fnc $$) { $H
    List *next = NULL;

	assert(NULL != free_list_fnc);

	/* Free up the list. */
    while(NULL != L) {
        next = ((List *) L)->_next;
        free_list_fnc(L _$$);
        L = next;
    }

    return_with;
}

/**
 * Set the next element in a linked list.
 */
void list_set_next(void *L, void *N $$) { $H
    ((List *) L)->_next = (List *) N;
    return_with;
}

/**
 * Get the next element of a linked list.
 */
List *list_get_next(void *L $$) { $H
	assert(NULL != L);
    return_with ((List *) L)->_next;
}

/**
 * Allocate a generic list on the heap.
 */
GenericList *gen_list_alloc($) { $H
    GenericList *L = list_alloc(sizeof(GenericList) _$$);
    L->_elm = NULL;
    return_with L;
}

/**
 * Free the allocated space of a generic list.
 */
void gen_list_free(GenericList *L, D1_t free_elm_fnc $$) { $H
    GenericList *next = NULL;

    assert(NULL != free_elm_fnc);

	/* Go through the chain and free the lists and their respective
	 * elements. */
    while(NULL != L) {
        next = (GenericList *) list_get_next(L _$$);
        free_elm_fnc(L->_elm _$$);
        L->_elm = NULL;
        mem_free(L);
        L = next;
    }

    return_with;
}

/**
 * Free only the element of a single list.
 */
void gen_list_free_elm(GenericList *L, D1_t free_elm_fnc $$) { $H
	assert(NULL != L);
    free_elm_fnc(L->_elm _$$);
    return_with;
}

/**
 * Get the list element from a generic list.
 */
void *gen_list_get_elm(GenericList *L $$) { $H
    return_with L->_elm;
}

/**
 * Set the list element from a generic list.
 */
void gen_list_set_elm(GenericList *L, void *elm $$) { $H
    L->_elm = elm;
    return_with;
}
