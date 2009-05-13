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
void *stack_alloc(const size_t struct_size ) { $H
    PStack *S = NULL;
    void *stack = NULL;

    assert(sizeof(PStack) <= struct_size);

    stack = mem_alloc(struct_size);

    if(NULL == stack) {
        mem_error("Unable to allocate a new stack on the heap.");
    }

    S = (PStack *) stack;
    S->_head = NULL;
    S->_unused = NULL;

    return_with stack;
}

/**
 * Empty a stack.
 */
void stack_empty(PStack *S, PDelegate free_elm_fnc ) { $H
    PGenericList *L = NULL,
                *next = NULL;

	assert_not_null(S);
	assert_not_null(free_elm_fnc);

	if(NULL == S->_head) {
        return_with;
	}

    L = S->_head;

    /* free up the elements in the stack and move the slots onto the unused
     * list */
    while(NULL != L) {
        gen_list_free_elm(L, free_elm_fnc );
        next = (PGenericList *) list_get_next(L);
        list_set_next(L, S->_unused );
        S->_unused = L;
        L = next;
    }

    S->_head = NULL;
    return_with;
}

/**
 * Free a stack.
 */
void stack_free(PStack *S, PDelegate free_elm_fnc ) { $H
	assert_not_null(S);
	assert_not_null(free_elm_fnc);
	
    gen_list_free(S->_head, free_elm_fnc );
    gen_list_free(S->_unused, &delegate_do_nothing );
    mem_free(S);
    
	S = NULL;
    return_with;
}

/**
 * Check if a stack is empty.
 */
char stack_is_empty(const PStack * const S ) { $H
	assert_not_null(S);
    return_with (NULL == S->_head);
}

/**
 * Push an element onto the stack.
 */
void stack_push(PStack * const S, void * E ) { $H
    PGenericList *L = NULL;

    assert_not_null(S);

    /* allocate a slot if needed */
    if(NULL == S->_unused) {
        L = gen_list_alloc();

    /* take the first unused one otherwise */
    } else {
        L = S->_unused;
        S->_unused = (PGenericList *) list_get_next(L);
    }

    /* add in the list to the head of the stack */
    list_set_next(L, S->_head );
    S->_head = L;
    gen_list_set_elm(L, E );

    return_with;
}

/**
 * Pop an element off of the stack.
 */
void *stack_pop(PStack * const S ) { $H
    void *E = NULL;
    PGenericList *L = NULL;

	assert(!stack_is_empty(S));

    /* extract the element */
    L = S->_head;
    E = gen_list_get_elm(L );

    /* update the head pointer */
    S->_head = (PGenericList *) list_get_next(L);

    /* keep the list around for future use */
    gen_list_set_elm(L, NULL );
    list_set_next(L, S->_unused );
    S->_unused = L;

    return_with E;
}

/**
 * Peek at the top element on the stack.
 */
void *stack_peek(const PStack * const S ) { $H
	assert(!stack_is_empty(S));
    return_with gen_list_get_elm(S->_head );
}
