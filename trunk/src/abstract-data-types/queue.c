/*
 * queue.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "list.h"
#include "stack.h"
#include "queue.h"
#include "delegate.h"

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
    stack_free(Q);
    Q = NULL;
}

/**
 * Check if the queue is empty.
 */
inline int queue_empty(const Queue * const Q) {
    return stack_empty(Q);
}

/**
 * Dequeue an element off of a queue.
 */
inline void *queue_pop(Queue * const Q) {
    return stack_pop(Q);
}

/**
 * Peek at the first element added to the queue.
 */
inline void *queue_peek(const Queue * const Q) {
    return stack_peek(Q);
}

/**
 * Enqueue an element onto a queue.
 */
void queue_push(Queue * const Q, const void * const E) {

    GenericList *L = stack_alloc_list(Q),
                *H = NULL;

    Stack *S = (Stack *)Q;

    if(NULL == L)
        return;

    L->elm = E;

    if(NULL != Q->tail) {
        ((List *) (Q->tail))->next = L;
        H = Q->tail;
    } else {
        H = L;
    }
    Q->tail = L;

    if(NULL == S->head)
        S->head = H;
}
