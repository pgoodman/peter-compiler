/*
 * queue.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include "std-include.h"
#include "func-delegate.h"
#include "adt-list.h"
#include "adt-stack.h"

typedef struct Queue {
    GenericList *_head,
                *_tail,
                *_unused;
} Queue;

void *queue_alloc(size_t $$);
void queue_free(Queue *, D1_t $$);
void queue_empty(Queue *, D1_t $$);
char queue_is_empty(const Queue * const $$);
void *queue_pop(Queue * const $$);
void *queue_peek(const Queue * const $$);
void queue_push(Queue *, void * $$);

#endif /* QUEUE_H_ */
