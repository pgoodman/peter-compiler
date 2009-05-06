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
void *queue_alloc(int struct_size) {
    void *queue = NULL;
    Queue *Q;

    if(struct_size < sizeof(Queue))
        struct_size = sizeof(Queue);

    queue = mem_alloc(struct_size MEM_DEBUG_INFO);
    if(NULL == queue)
        mem_error("Unable to allocate a new queue on the heap.");

    Q = (Queue *) queue;
    Q->_head = NULL;
    Q->_tail = NULL;
    Q->_unused = NULL;

    return Q;
}

/**
 * Free an allocated queue.
 */
void queue_free(Queue *Q, D1 free_elm) {
    if(NULL == Q)
        return;

    if(NULL == free_elm)
        free_elm = &D1_ignore;

    Q->_tail = NULL;

    gen_list_free(Q->_head, free_elm);
    gen_list_free(Q->_unused, &D1_ignore);
    mem_free(Q MEM_DEBUG_INFO);

    Q = NULL;
}

/**
 * Check if the queue is empty.
 */
int queue_empty(const Queue * const Q) {
    return NULL == Q->_head;
}

/**
 * Push an element onto the queue.
 */
void queue_push(Queue * const Q, void * E) {
    GenericList *L = NULL;

    if(NULL == Q)
        return;

    // allocate a slot if needed
    if(NULL == Q->_unused) {
        L = gen_list_alloc();

    // take the first unused one otherwise
    } else {
        L = Q->_unused;
        Q->_unused = (GenericList *) list_get_next(L);
    }

    // add in the slot to the tail of the queue
    if(NULL != Q->_tail) {
        list_set_next(Q->_tail, L);

    // the tail is null <==> the head is null
    } else {
        Q->_head = L;
    }

    // update the to the new tail
    Q->_tail = L;
    gen_list_set_elm(L, E);
}

/**
 * Dequeue an element from the queue.
 */
void *queue_pop(Queue * const Q) {
    void *E = NULL;
    GenericList *L = NULL;

    if(queue_empty(Q))
        return NULL;

    // extract the element
    L = Q->_head;
    E = gen_list_get_elm(L);

    // clear out the tail pointer if necessary if the head was the tail
    if(Q->_tail == L) {
        Q->_tail = NULL;
        Q->_head = NULL;

    // update the head pointer
    } else {
        Q->_head = (GenericList *) list_get_next(L);
    }

    // keep the list around for future use
    gen_list_set_elm(L, NULL);
    list_set_next(L, Q->_unused);
    Q->_unused = L;

    return E;
}
