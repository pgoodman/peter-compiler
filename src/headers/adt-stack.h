/*
 * stack.h
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef STACK_H_
#define STACK_H_

#include "std-include.h"
#include "func-delegate.h"
#include "adt-list.h"

typedef struct PStack {
    PGenericList *_head,
                *_unused;
} PStack;

void *stack_alloc(const size_t );
void stack_free(PStack *, PDelegate );
void stack_empty(PStack *, PDelegate );
char stack_is_empty(const PStack * const );
void stack_push(PStack * const, void * );
void *stack_pop(PStack * const );
void *stack_peek(const PStack * const );

#endif /* STACK_H_ */
