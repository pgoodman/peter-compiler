/*
 * stack.h
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef STACK_H_
#define STACK_H_

#include <stdheader.h>
#include "list.h"
#include "delegate.h"

typedef struct Stack {
    GenericList *_head,
                *_unused;
} Stack;

void *stack_alloc(size_t);
void stack_free(Stack *, D1_t);
void stack_empty(Stack *, D1_t);
char stack_is_empty(const Stack * const);
void stack_push(Stack * const, void *);
void *stack_pop(Stack * const);
void *stack_peek(const Stack * const);

#endif /* STACK_H_ */
