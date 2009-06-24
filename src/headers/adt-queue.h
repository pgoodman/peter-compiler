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

typedef struct PQueue {
    PGenericList *_head,
                *_tail,
                *_unused;
} PQueue;

void *queue_alloc(const size_t );
void queue_free(PQueue *, PDelegate *);
void queue_empty(PQueue *, PDelegate *);
char queue_is_empty(const PQueue * const );
void *queue_pop(PQueue * const );
void *queue_peek(const PQueue * const );
void queue_push(PQueue *, void * );

#endif /* QUEUE_H_ */
