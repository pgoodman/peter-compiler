/*
 * tree.c
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-tree.h>

#define FREE_GENERATOR_FUNC(N, T, F) \
    static void N(void *g $$) { $H\
        PTreeGenerator *G = NULL; \
        T *A = NULL; \
        assert_not_null(g); \
        G = (PTreeGenerator *) g; \
        A = (T *) (G->_adt); \
        G->_adt = NULL; \
        F(A, D1_ignore $$A); \
        mem_free(g); \
        G = NULL; \
        g = NULL; \
        return_with; \
    }

/**
 * Allocate a new N-ary tree on the heap. The struct_size is the size of the
 * structure which *must* contain a PTree as its first field, and the degree
 * is the number of branches that this tree should have.
 */
void *tree_alloc(const size_t struct_size, const unsigned short degree $$) { $H
    void *tree = NULL;
    PTree *T = NULL,
         **B = NULL;

    assert(sizeof(PTree) <= struct_size);

    tree = mem_alloc(struct_size);

    if(NULL == tree) {
        mem_error("Unable to allocate a tree on the heap.");
    }

    T = (PTree *) tree;
    if(0 < degree) {
        B = mem_alloc(struct_size * degree);

        if(NULL == B) {
            mem_error("Unable to allocate the branches for a tree on the heap.");
        }
    }

	/* initialize the tree fields. */
    T->_degree = degree;
    T->_fill = 0;
    T->_branches = B;
    T->_parent_branch = 0;
    T->_parent = NULL;

    return_with tree;
}

/**
 * Figure out a valid memory freeing callback for freeing things.
 */
static PDelegate T_valid_free_callback(void * ft, PDelegate disallowed, PDelegate allowed $$) { $H
    PDelegate free_tree = NULL;

    /* make sure we don't over free */
    assert_not_null(ft);

    if((void *)disallowed == ft)
        free_tree = allowed;

    if(NULL == free_tree)
        free_tree = (PDelegate) ft;

    return_with free_tree;
}

/**
 * Free the pointers of a tree. This takes in a visitor call-back that can
 * perform any manual freeing on the tree.
 *
 * !!! The call-back is responsible ONLY for freeing any heap allocated fields
 *     which are not part of the PTree structure. The PTree structure should be
 *     seen as a black box and not touched.
 */
void tree_free(void *T, PDelegate free_tree_fnc $$) { $H

    PTreeGenerator *G = NULL;
    PTree *node = NULL;
    void *elm = NULL;

    assert_not_null(T);

    /* get a valid memory free callback for this context */
    free_tree_fnc = T_valid_free_callback(
        free_tree_fnc,
        &D1_mem_free, /* not allowed */
        &D1_ignore $$A /* alternative to above */
    );

    /* traverse the tree in post order and free the tree nodes from the
     * bottom up. */
    G = tree_generator_alloc(T, TREE_TRAVERSE_POSTORDER $$A);

    while(generator_next(G $$A)) {
        elm = generator_current(G $$A);
        node = (PTree *)elm;

        if(0 < node->_degree) {
            mem_free(node->_branches);
        }

        free_tree_fnc(elm $$A);
        mem_free(elm);
    }

    /* free the generator */
    generator_free(G $$A);
    G = NULL;
    T = NULL;

    return_with;
}

/**
 * A tree free with no callback.
 */
void PDelegateree_free(void *T $$) { $H
    tree_free(T, &D1_ignore $$A);
    return_with;
}

/**
 * Return the number of branches allocated for this tree.
 */
size_t tree_degree(void *t $$) { $H
    assert_not_null(t);
    return_with ((PTree *) t)->_degree;
}

/**
 * Return the number of allocated branches that have been filled.
 */
size_t tree_fill(void *t $$) { $H
    assert_not_null(t);
    return_with ((PTree *) t)->_fill;
}

/**
 * Trim off all of the branches, and push each of them into the garbage stack.
 */
void tree_trim(void *tree, PStack *garbage_stack $$) { $H

	assert_not_null(tree);
	assert_not_null(garbage_stack);

    PTree *T = (PTree *) tree,
         *branch = NULL;

    unsigned short i;

    /* none of the branches have been filled, or no branches were allocated,
     * so we have nothing to do.. yay! */
    if(T->_fill == 0 || NULL == T->_branches) {
        return_with;
    }

    /* sub-trees are there, collect them into our garbage collection. */
    for(i = 0; i < T->_fill; ++i) {
        branch = (PTree *) (T->_branches[i]);
        stack_push(garbage_stack, branch $$A);

        /* update the branches parent information */
        branch->_parent_branch = 0;
        branch->_parent = NULL;
        T->_branches[i] = NULL;
    }

    /* update the tree accordingly */
    T->_fill = 0;

    return_with;
}

/**
 * Set a tree C as one of the branches of T. Return 1 if successful, 0 on failure.
 */
char tree_add_branch(void *tree, void *branch $$) { $H

	assert_not_null(tree);
	assert_not_null(branch);

    PTree *parent = (PTree *) tree,
         *child = (PTree *) branch;

    assert(parent->_fill < parent->_degree);

    parent->_branches[parent->_fill] = child;
    ++(parent->_fill);

    return_with 1;
}

FREE_GENERATOR_FUNC(T_generator_free_s, PStack, stack_free)
FREE_GENERATOR_FUNC(T_generator_free_q, PQueue, queue_free)

/**
 * Generate the next tree element in a depth-first traversal of the tree.
 */
static void *T_generator_next_df(void *g $$) { $H
    PStack *S = NULL;
    PTree *curr = NULL;
    void *ret = NULL;
    unsigned short i;
    PTreeGenerator *G = NULL;

	assert_not_null(g);

    G = (PTreeGenerator *) g;
    S = (PStack *) G->_adt;

    while(!stack_is_empty(S $$A)) {

        ret = stack_pop(S $$A);
        curr = (PTree *)ret;

        if(NULL == curr)
            continue;

        /* push the branches onto the stack */
        for(i = curr->_fill; i > 0; ) {
            stack_push(S, curr->_branches[--i] $$A);
        }

        return_with ret;
    }

    return_with NULL;
}

/**
 * Generate the next tree element in a breadth-first traversal of the tree.
 */
static void *T_generator_next_bf(void *g $$) { $H
    PQueue *Q = NULL;
    PTree *curr = NULL;
    void *ret = NULL;
    unsigned short i;
    PTreeGenerator *G = NULL;

	assert_not_null(g);

    G = (PTreeGenerator *) g;
    Q = (PQueue *) G->_adt;

    while(!queue_is_empty(Q $$A)) {
        ret = queue_pop(Q $$A);
        curr = (PTree *)ret;

        if(NULL == curr)
            continue;

        /* push the branches onto the queue */
        for(i = 0; i < curr->_fill; ++i) {
            queue_push(Q, curr->_branches[i] $$A);
        }

        return_with ret;
    }

    return_with NULL;
}

/**
 * Generate the nodes of a post-order traversal of a tree one at a time.
 */
static void *T_generator_next_po(void *g $$) { $H
    PStack *S = NULL;
    PTree *curr = NULL,
         *top;
    void *ret = NULL;
    unsigned short i;
    PTreeGenerator *G = NULL;

	assert_not_null(g);

    G = (PTreeGenerator *) g;
    S = (PStack *) G->_adt;

    if(stack_is_empty(S $$A)) {
        return_with NULL;
    }

    while(!stack_is_empty(S $$A)) {

        curr = stack_peek(S $$A);

        /* subtrees have been explored */
        if(NULL == curr) {

            /* get the tree to return */
            stack_pop(S $$A);
            ret = stack_pop(S $$A);
            curr = (PTree *)ret;

            /* we are looking at the last element that is a direct descendant
             * of the tree on the top of the stack, push NULL on the stack to
             * notify us to ignore this nodes children in future. */
            if(!stack_is_empty(S $$A)) {

                top = (PTree *) stack_peek(S $$A);

                if(NULL == top)
                    break;

                if(top->_fill == 0 || top->_branches[top->_fill - 1] == curr) {
                    stack_push(S, NULL $$A);
                }
            }

            break;

        /* subtrees weren't explored, explore them! */
        } else {
            do {
                /* add in the children of this node to the stack */
                if(curr->_fill > 0) {
                    for(i = curr->_fill; i > 0; ) {
                        stack_push(S, curr->_branches[--i] $$A);
                    }

                    curr = stack_peek(S $$A);

                /* no sub-trees to explore, this is a leaf node */
                } else {
                    stack_push(S, NULL $$A);
                    break;
                }

            } while(NULL != curr);
        }
    }

    return_with ret;
}

/**
 * Allocate a tree generator on the heap and initialize that generator.
 */
PTreeGenerator *tree_generator_alloc(void *tree,
                                    const PTreeTraversal traverse_type $$) { $H
	assert_not_null(tree);

    /* allocate the generator */
    PTree *T = (PTree *) tree;
    PTreeGenerator *G = generator_alloc(sizeof(PTreeGenerator) $$A);

    G->_adt = NULL;

    /* initialize the various types of generators */
    switch(traverse_type) {

        /* post-order traversal, doesn't use the generic ADT methods of the
         * generator */
        case TREE_TRAVERSE_POSTORDER:
            G->_adt = stack_alloc(sizeof(PStack) $$A);
            generator_init(G, \
                &T_generator_next_po, \
                &T_generator_free_s \
                $$A
            );
            stack_push(G->_adt, T $$A);
            break;

        /* depth-first traversal */
        case TREE_TRAVERSE_PREORDER:
            G->_adt = stack_alloc(sizeof(PStack) $$A);
            generator_init(G, \
                &T_generator_next_df, \
                &T_generator_free_s \
                $$A
            );
            stack_push(G->_adt, T $$A);
            break;

        /* breadth-first traversal */
        case TREE_TRAVERSE_LEVELORDER:
            G->_adt = queue_alloc(sizeof(PQueue) $$A);
            generator_init(G, \
                &T_generator_next_bf, \
                &T_generator_free_q \
                $$A
            );
            queue_push(G->_adt, T $$A);
            break;
    }

    return_with G;
}
