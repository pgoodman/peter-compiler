/*
 * stack.h
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef STACK_H_
#define STACK_H_

typedef struct Stack {
    GenericList *head,
                *unused;
} Stack;

Stack *stack_alloc(void);
void stack_free(Stack *, D1);
List *stack_alloc_list(Stack * const);

inline int stack_empty(const Stack * const);
void stack_push(Stack * const, const void * const);
void *stack_pop(Stack * const);
void *stack_peek(const Stack * const);

#endif /* STACK_H_ */
