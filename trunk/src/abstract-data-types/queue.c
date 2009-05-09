/*
 * queue.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "queue.h"

/**
 * Allocate a new queue on the heap.
 */
void *queue_alloc(size_t struct_size) {
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

    return Q;
}

/**
 * Empty a queue of its elements.
 */
void queue_empty(Queue *Q, D1_t free_elm_fnc) {
    GenericList *L,
                *next;
	
	assert(NULL != Q && NULL != free_elm_fnc);
	
	if(NULL == Q->_head)
		return;
	
    L = Q->_head;

    /* free up the elements in the queue and move the slots onto the unused
     * list */
    while(NULL != L) {
        gen_list_free_elm(L, free_elm_fnc);
        next = (GenericList *) list_get_next(L);
        list_set_next(L, Q->_unused);
        Q->_unused = L;
        L = next;
    }

    Q->_head = NULL;
    Q->_tail = NULL;
}

/**
 * Free an allocated queue.
 */
void queue_free(Queue *Q, D1_t free_elm) {
    assert(NULL != Q && NULL != free_elm);

    Q->_tail = NULL;

    gen_list_free(Q->_head, free_elm);
    gen_list_free(Q->_unused, &D1_ignore);
    mem_free(Q);

    Q = NULL;
}

/**
 * Check if the queue is empty.
 */
char queue_is_empty(const Queue * const Q) {
	assert(NULL != Q);
    return NULL == Q->_head;
}

/**
 * Push an element onto the queue.
 */
void queue_push(Queue * const Q, void * E) {
    GenericList *L = NULL;

	assert(NULL != Q);

    /* allocate a slot if needed */
    if(NULL == Q->_unused) {
        L = gen_list_alloc();

    /* take the first unused one otherwise */
    } else {
        L = Q->_unused;
        Q->_unused = (GenericList *) list_get_next(L);
    }

    /* add in the slot to the tail of the queue */
    if(NULL != Q->_tail) {
        list_set_next(Q->_tail, L);

    /* the tail is null <==> the head is null */
    } else {
        Q->_head = L;
    }

    /* update the to the new tail */
    Q->_tail = L;
    gen_list_set_elm(L, E);
}

/**
 * Dequeue an element from the queue.
 */
void *queue_pop(Queue * const Q) {
    void *E = NULL;
    GenericList *L = NULL;

    assert(!queue_is_empty(Q));

    /* extract the element */
    L = Q->_head;
    E = gen_list_get_elm(L);

    /* clear out the tail pointer if necessary if the head was the tail */
    if(Q->_tail == L) {
        Q->_tail = NULL;
        Q->_head = NULL;

    /* update the head pointer */
    } else {
        Q->_head = (GenericList *) list_get_next(L);
    }

    /* keep the list around for future use */
    gen_list_set_elm(L, NULL);
    list_set_next(L, Q->_unused);
    Q->_unused = L;

    return E;
}
