/*
 * stack.c
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-stack.h>

/**
 * Allocate a new generic stack on the heap.
 */
void *stack_alloc(size_t struct_size $$) { $H
    Stack *S;
    void *stack;

    if(struct_size < sizeof(Stack))
        struct_size = sizeof(Stack);

    stack = mem_alloc(struct_size);
    if(NULL == stack)
        mem_error("Unable to allocate a new stack on the heap.");

    S = (Stack *) stack;
    S->_head = NULL;
    S->_unused = NULL;

    return_with stack;
}

/**
 * Empty a stack.
 */
void stack_empty(Stack *S, D1_t free_elm_fnc $$) { $H
    GenericList *L,
                *next;

	assert(NULL != S && NULL != free_elm_fnc);

	if(NULL == S->_head)
        return_with;

    L = S->_head;

    /* free up the elements in the stack and move the slots onto the unused
     * list */
    while(NULL != L) {
        gen_list_free_elm(L, free_elm_fnc _$$);
        next = (GenericList *) list_get_next(L _$$);
        list_set_next(L, S->_unused _$$);
        S->_unused = L;
        L = next;
    }

    S->_head = NULL;
    return_with;
}

/**
 * Free a stack.
 */
void stack_free(Stack *S, D1_t free_elm_fnc $$) { $H
    assert(NULL != S && NULL != free_elm_fnc);
    gen_list_free(S->_head, free_elm_fnc _$$);
    gen_list_free(S->_unused, &D1_ignore _$$);
    mem_free(S);
    S = NULL;
    return_with;
}

/**
 * Check if a stack is empty.
 */
char stack_is_empty(const Stack * const S $$) { $H
	assert(NULL != S);
    return_with (NULL == S->_head);
}

/**
 * Push an element onto the stack.
 */
void stack_push(Stack * const S, void * E $$) { $H
    GenericList *L = NULL;

    assert(NULL != S);

    /* allocate a slot if needed */
    if(NULL == S->_unused) {
        L = gen_list_alloc(_$);

    /* take the first unused one otherwise */
    } else {
        L = S->_unused;
        S->_unused = (GenericList *) list_get_next(L _$$);
    }

    /* add in the list to the head of the stack */
    list_set_next(L, S->_head _$$);
    S->_head = L;
    gen_list_set_elm(L, E _$$);

    return_with;
}

/**
 * Pop an element off of the stack.
 */
void *stack_pop(Stack * const S $$) { $H
    void *E = NULL;
    GenericList *L = NULL;

	assert(!stack_is_empty(S _$$));

    /* extract the element */
    L = S->_head;
    E = gen_list_get_elm(L _$$);

    /* update the head pointer */
    S->_head = (GenericList *) list_get_next(L _$$);

    /* keep the list around for future use */
    gen_list_set_elm(L, NULL _$$);
    list_set_next(L, S->_unused _$$);
    S->_unused = L;

    return_with E;
}

/**
 * Peek at the top element on the stack.
 */
void *stack_peek(const Stack * const S $$) { $H
	assert(!stack_is_empty(S _$$));
    return_with gen_list_get_elm(S->_head _$$);
}
