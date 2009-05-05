/*
 * queue.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct Queue {
    Stack;
    GenericList *tail;
} Queue;

Queue *queue_alloc(void);
inline void queue_free(Queue *, D1);
inline int queue_empty(const Queue * const);
inline void *queue_pop(Queue * const);
inline void *queue_peek(const Queue * const);
void queue_push(Queue * const, const void * const);

#endif /* QUEUE_H_ */
