/*
 * stack.h
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef STACK_H_
#define STACK_H_

#include <stdlib.h>
#include "mem.h"
#include "list.h"
#include "delegate.h"

typedef struct Stack {
    GenericList *head,
                *unused;
} Stack;

Stack *stack_alloc(void);
void stack_free(Stack *, D1);
GenericList *stack_alloc_list(Stack *);

int stack_empty(const Stack * const);
void stack_push(Stack * const, void *);
void *stack_pop(Stack * const);
void *stack_peek(const Stack * const);

#endif /* STACK_H_ */
