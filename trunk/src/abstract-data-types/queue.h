/*
 * queue.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdlib.h>
#include <memory-management/static-mem.h>
#include "list.h"
#include "stack.h"
#include "delegate.h"

typedef struct Queue {
    GenericList *_head,
                *_tail,
                *_unused;
} Queue;

void *queue_alloc(int);
void queue_free(Queue *, D1);
int queue_empty(const Queue * const);
void *queue_pop(Queue * const);
void *queue_peek(const Queue * const);
void queue_push(Queue *, void *);

#endif /* QUEUE_H_ */
