/*
 * queue.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-queue.h>

/**
 * Allocate a new queue on the heap.
 */
void *queue_alloc(size_t struct_size $$) { $H
    void *queue = NULL;
    Queue *Q;

    if(struct_size < sizeof(Queue))
        struct_size = sizeof(Queue);

    queue = mem_alloc(struct_size);
    if(NULL == queue)
        mem_error("Unable to allocate a new queue on the heap.");

    Q = (Queue *) queue;
    Q->_head = NULL;
    Q->_tail = NULL;
    Q->_unused = NULL;

    return_with Q;
}

/**
 * Empty a queue of its elements.
 */
void queue_empty(Queue *Q, D1_t free_elm_fnc $$) { $H
    GenericList *L,
                *next;

	assert(NULL != Q && NULL != free_elm_fnc);

	if(NULL == Q->_head)
		return_with;

    L = Q->_head;

    /* free up the elements in the queue and move the slots onto the unused
     * list */
    while(NULL != L) {
        gen_list_free_elm(L, free_elm_fnc _$$);
        next = (GenericList *) list_get_next(L _$$);
        list_set_next(L, Q->_unused _$$);
        Q->_unused = L;
        L = next;
    }

    Q->_head = NULL;
    Q->_tail = NULL;

    return_with;
}

/**
 * Free an allocated queue.
 */
void queue_free(Queue *Q, D1_t free_elm $$) { $H
    assert(NULL != Q && NULL != free_elm);

    Q->_tail = NULL;

    gen_list_free(Q->_head, free_elm _$$);
    gen_list_free(Q->_unused, &D1_ignore _$$);
    mem_free(Q);

    Q = NULL;

    return_with;
}

/**
 * Check if the queue is empty.
 */
char queue_is_empty(const Queue * const Q $$) { $H
	assert(NULL != Q);
    return_with NULL == Q->_head;
}

/**
 * Push an element onto the queue.
 */
void queue_push(Queue * const Q, void * E $$) { $H
    GenericList *L = NULL;

	assert(NULL != Q);

    /* allocate a slot if needed */
    if(NULL == Q->_unused) {
        L = gen_list_alloc(_$);

    /* take the first unused one otherwise */
    } else {
        L = Q->_unused;
        Q->_unused = (GenericList *) list_get_next(L _$$);
    }

    /* add in the slot to the tail of the queue */
    if(NULL != Q->_tail) {
        list_set_next(Q->_tail, L _$$);

    /* the tail is null <==> the head is null */
    } else {
        Q->_head = L;
    }

    /* update the to the new tail */
    Q->_tail = L;
    gen_list_set_elm(L, E _$$);

    return_with;
}

/**
 * Dequeue an element from the queue.
 */
void *queue_pop(Queue * const Q $$) { $H
    void *E = NULL;
    GenericList *L = NULL;

    assert(!queue_is_empty(Q _$$));

    /* extract the element */
    L = Q->_head;
    E = gen_list_get_elm(L _$$);

    /* clear out the tail pointer if necessary if the head was the tail */
    if(Q->_tail == L) {
        Q->_tail = NULL;
        Q->_head = NULL;

    /* update the head pointer */
    } else {
        Q->_head = (GenericList *) list_get_next(L _$$);
    }

    /* keep the list around for future use */
    gen_list_set_elm(L, NULL _$$);
    list_set_next(L, Q->_unused _$$);
    Q->_unused = L;

    return_with E;
}
