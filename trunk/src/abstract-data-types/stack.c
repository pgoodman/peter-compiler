/*
 * stack.c
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <stdio.h>
#include "stack.h"

/**
 * Allocate a new stack on the heap.
 */
Stack *stack_alloc(void) {
    Stack *S = mem_alloc(sizeof(Stack) MEM_DEBUG_INFO);

    if(NULL == S)
        mem_error("Unable to allocate a new stack on the heap.");

    S->head = NULL;
    S->unused = NULL;

    return S;
}

/**
 * Free a stack.
 */
void stack_free(Stack *S, D1 free_elm) {
    if(NULL == S)
        return;

    if(NULL == free_elm)
        free_elm = &D1_ignore;

    printf("freeing S->head\n");
    gen_list_free(S->head, free_elm);
    printf("freeing S->unused\n");
    gen_list_free(S->unused, &D1_ignore);
    mem_free(S MEM_DEBUG_INFO);

    S = NULL;
}

/**
 * Allocate a new stack list.
 */
GenericList *stack_alloc_list(Stack * const S) {
    GenericList *L = NULL;

    if(NULL == S)
        return NULL;

    // allocate a new list
    if(NULL == S->unused) {
        L = gen_list_alloc();

    // use an available one
    } else {
        L = S->unused;
        S->unused = (GenericList *) (((List *) L)->next);
        ((List *) L)->next = NULL;
    }

    return L;
}

/**
 * Check if a stack is empty.
 */
int stack_empty(const Stack * const S) {
    return NULL == S || NULL == S->head;
}

/**
 * Push an element onto the stack.
 */
void stack_push(Stack * const S, void * E) {
    GenericList *L = stack_alloc_list(S);

    if(NULL == L)
        return;

    // add in the list to the head of the stack
    ((List *) L)->next = (List *)S->head;
    S->head = L;
    L->elm = E;
}

/**
 * Pop an element off of the stack.
 */
void *stack_pop(Stack * const S) {
    void *E = NULL;
    GenericList *L = NULL;

    if(stack_empty(S))
        return NULL;

    // extract the element
    L = S->head;
    E = L->elm;

    // update the head pointer
    S->head = (GenericList *) (((List *) L)->next);

    // keep the list around for future use
    L->elm = NULL;
    ((List *) L)->next = S->unused;
    S->unused = L;

    return E;
}

/**
 * Peek at the top element on the stack.
 */
void *stack_peek(const Stack * const S) {
    if(stack_empty(S))
        return NULL;

    return S->head->elm;
}
