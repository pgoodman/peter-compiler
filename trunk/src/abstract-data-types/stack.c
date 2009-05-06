/*
 * stack.c
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "stack.h"

/**
 * Allocate a new generic stack on the heap.
 */
void *stack_alloc(int struct_size) {
    Stack *S;
    void *stack;

    if(struct_size < sizeof(Stack))
        struct_size = sizeof(Stack);

    stack = mem_alloc(struct_size MEM_DEBUG_INFO);
    if(NULL == stack)
        mem_error("Unable to allocate a new stack on the heap.");

    S = (Stack *) stack;
    S->_head = NULL;
    S->_unused = NULL;

    return stack;
}

/**
 * Free a stack.
 */
void stack_free(Stack *S, D1 free_elm) {
    if(NULL == S)
        return;

    if(NULL == free_elm)
        free_elm = &D1_ignore;

    gen_list_free(S->_head, free_elm);
    gen_list_free(S->_unused, &D1_ignore);
    mem_free(S MEM_DEBUG_INFO);

    S = NULL;
}

/**
 * Check if a stack is empty.
 */
int stack_empty(const Stack * const S) {
    return NULL == S || NULL == S->_head;
}

/**
 * Push an element onto the stack.
 */
void stack_push(Stack * const S, void * E) {
    GenericList *L = NULL;

    if(NULL == S)
        return;

    // allocate a slot if needed
    if(NULL == S->_unused) {
        L = gen_list_alloc();

    // take the first unused one otherwise
    } else {
        L = S->_unused;
        S->_unused = (GenericList *) list_get_next(L);
    }

    // add in the list to the head of the stack
    list_set_next(L, S->_head);
    S->_head = L;
    gen_list_set_elm(L, E);
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
    L = S->_head;
    E = gen_list_get_elm(L);

    // update the head pointer
    S->_head = (GenericList *) list_get_next(L);

    // keep the list around for future use
    gen_list_set_elm(L, NULL);
    list_set_next(L, S->_unused);
    S->_unused = L;

    return E;
}

/**
 * Peek at the top element on the stack.
 */
void *stack_peek(const Stack * const S) {
    if(stack_empty(S))
        return NULL;

    return gen_list_get_elm(S->_head);
}
