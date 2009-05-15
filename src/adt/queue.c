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
void *queue_alloc(const size_t struct_size ) {
    void *queue = NULL;
    PQueue *Q = NULL;

    assert(sizeof(PQueue) <= struct_size);

    queue = mem_alloc(struct_size);
    if(is_null(queue)) {
        mem_error("Unable to allocate a new queue on the heap.");
    }

    Q = (PQueue *) queue;
    Q->_head = NULL;
    Q->_tail = NULL;
    Q->_unused = NULL;

    return queue;
}

/**
 * Empty a queue of its elements.
 */
void queue_empty(PQueue *Q, PDelegate free_elm_fnc ) {
    PGenericList *L = NULL,
                 *next = NULL;

	assert_not_null(Q);
	assert_not_null(free_elm_fnc);

	if(is_null(Q->_head)) {
		return;
	}

    L = Q->_head;

    /* free up the elements in the queue and move the slots onto the unused
     * list */
    while(is_not_null(L)) {
        gen_list_free_elm(L, free_elm_fnc );
        next = (PGenericList *) list_get_next(L);
        list_set_next(L, Q->_unused );
        Q->_unused = L;
        L = next;
    }

    Q->_head = NULL;
    Q->_tail = NULL;

    return;
}

/**
 * Free an allocated queue.
 */
void queue_free(PQueue *Q, PDelegate free_elm ) {
	assert_not_null(Q);
	assert_not_null(free_elm);

    Q->_tail = NULL;

    gen_list_free(Q->_head, free_elm );
    gen_list_free(Q->_unused, &delegate_do_nothing );
    mem_free(Q);

    Q = NULL;

    return;
}

/**
 * Check if the queue is empty.
 */
char queue_is_empty(const PQueue * const Q ) {
	assert_not_null(Q);
    return is_null(Q->_head);
}

/**
 * Push an element onto the queue.
 */
void queue_push(PQueue * const Q, void * E ) {
    PGenericList *L = NULL;

    assert_not_null(Q);

    /* allocate a slot if needed */
    if(is_null(Q->_unused)) {
        L = gen_list_alloc();

    /* take the first unused one otherwise */
    } else {
        L = Q->_unused;
        Q->_unused = (PGenericList *) list_get_next(L);
    }

    /* add in the slot to the tail of the queue */
    if(is_not_null(Q->_tail)) {
        list_set_next(Q->_tail, L );

    /* the tail is null <==> the head is null */
    } else {
        Q->_head = L;
    }

    /* update the to the new tail */
    Q->_tail = L;
    gen_list_set_elm(L, E );

    return;
}

/**
 * Dequeue an element from the queue.
 */
void *queue_pop(PQueue * const Q) {
    void *E = NULL;
    PGenericList *L = NULL;

    assert(!queue_is_empty(Q));

    /* extract the element */
    L = Q->_head;
    E = gen_list_get_elm(L );

    /* clear out the tail pointer if necessary if the head was the tail */
    if(Q->_tail == L) {
        Q->_tail = NULL;
        Q->_head = NULL;

    /* update the head pointer */
    } else {
        Q->_head = (PGenericList *) list_get_next(L);
    }

    /* keep the list around for future use */
    gen_list_set_elm(L, NULL );
    list_set_next(L, Q->_unused );
    Q->_unused = L;

    return E;
}
