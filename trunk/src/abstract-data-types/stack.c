/*
 * stack.c
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "list.h"
#include "stack.h"
#include "mem.h"
#include "delegate.h"

/**
 * Allocate a new stack on the heap.
 */
Stack *stack_alloc(void) {
    Stack *S = mem_alloc(sizeof(Stack));

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

    gen_list_free(S->head, free_elm);
    gen_list_free(S->unused, &D1_ignore);
    free(S);

    S = NULL;
}

/**
 * Allocate a new stack list.
 */
List *stack_alloc_list(Stack * const S) {
    GenericList *L;

    if(NULL == S)
        return NULL;

    // allocate a new list or use an available one
    if(NULL == S->unused) {
        L = gen_list_alloc();
    } else {
        L = S->unused;
        S->unused = ((List *) L)->next;
    }

    return L;
}

/**
 * Check if a stack is empty.
 */
inline int stack_empty(const Stack * const S) {
    return NULL == S || NULL == S->head;
}

/**
 * Push an element onto the stack.
 */
void stack_push(Stack * const S, const void * const E) {
    GenericList *L = stack_alloc_list(S);

    if(NULL == L)
        return;

    // add in the list to the head of the stack
    ((List *) L)->next = S->head;
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
