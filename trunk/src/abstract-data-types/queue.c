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
Queue *queue_alloc(void) {
    Queue *Q = mem_alloc(sizeof(Queue));
    Stack *S;

    if(NULL == Q)
        mem_error("Unable to allocate a new queue on the heap.");

    S = (Stack *)Q;
    S->head = NULL;
    S->unused = NULL;

    Q->tail = NULL;

    return Q;
}

/**
 * Free an allocated queue.
 */
inline void queue_free(Queue *Q, D1 free_elm) {
    stack_free((Stack *) Q, free_elm);
    Q = NULL;
}

/**
 * Check if the queue is empty.
 */
inline int queue_empty(const Queue * const Q) {
    return stack_empty((Stack *) Q);
}

/**
 * Dequeue an element off of a queue.
 */
inline void *queue_pop(Queue * const Q) {
    return stack_pop((Stack *) Q);
}

/**
 * Peek at the first element added to the queue.
 */
inline void *queue_peek(const Queue * const Q) {
    return stack_peek((Stack *) Q);
}

/**
 * Enqueue an element onto a queue.
 */
void queue_push(Queue * Q, void *E) {

    Stack *S = (Stack *) Q;
    GenericList *L = stack_alloc_list(S),
                *H = NULL;

    if(NULL == L)
        return;

    L->elm = E;

    // if we have a tail to this queue then add E after the tail and remember
    // what the tail was in H
    if(NULL != Q->tail) {
        ((List *) \
            ((GenericList *) \
                (Q->tail))) \
                    ->next = L;
        H = Q->tail;
    } else {
        H = L;
    }

    Q->tail = L;

    // we don't have a head to the list so set the head to what the old tail was
    if(NULL == S->head)
        S->head = H;
}
