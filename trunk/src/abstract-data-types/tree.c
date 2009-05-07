/*
 * tree.c
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "tree.h"

#define FREE_GENERATOR_FUNC(N, T, F) \
    static void N(void *g) { \
        TreeGenerator *G = NULL; \
        T *A = NULL; \
        if(NULL == g) \
            return; \
        G = (TreeGenerator *) g; \
        A = (T *) (G->_adt); \
        G->_adt = NULL; \
        F(A, D1_ignore); \
        mem_free(g MEM_DEBUG_INFO); \
        G = NULL; \
        g = NULL; \
    }

/**
 * Allocate a new N-ary tree on the heap. The struct_size is the size of the
 * structure which *must* contain a Tree as its first field, and the degree
 * is the number of branches that this tree should have.
 */
void *tree_alloc(int struct_size, const size_t degree) {
    void *tree = NULL;
    Tree *T = NULL,
         **B = NULL;

    if(struct_size < sizeof(Tree))
        struct_size = sizeof(Tree);

    tree = mem_alloc(struct_size MEM_DEBUG_INFO);
    if(NULL == tree)
        mem_error("Unable to allocate a tree on the heap.");

    T = (Tree *)tree;

    if(0 < degree) {
        B = mem_alloc(struct_size * degree MEM_DEBUG_INFO);
        if(NULL == B)
            mem_error("Unable to allocate the branches for a tree on the heap.");
    }

    T->_degree = degree;
    T->_fill = 0;
    T->_branches = B;
    T->_parent_branch = 0;
    T->_parent = NULL;

    return tree;
}

/**
 * Figure out a valid memory freeing callback for freeing things.
 */
static D1 T_valid_free_callback(void * ft, D1 disallowed, D1 allowed) {
    D1 free_tree = NULL;

    // make sure we don't over free
    if(NULL == ft)
        free_tree = &D1_ignore;

    if((void *)&mem_free == ft)
        free_tree = &mem_free_no_debug;

    if((void *)disallowed == ft)
        free_tree = allowed;

    if(NULL == free_tree)
        free_tree = (D1) ft;

    return free_tree;
}

/**
 * Free the pointers of a tree. This takes in a visitor call-back that can
 * perform any manual freeing on the tree.
 *
 * !!! The call-back is responsible ONLY for freeing any heap allocated fields
 *     which are not part of the Tree structure. The Tree structure should be
 *     seen as a black box and not touched.
 */
void tree_free(void *T, D1 free_tree_fnc) {

    TreeGenerator *G = NULL;
    Tree *node = NULL;
    void *elm = NULL;

    if(NULL == T)
        return;

    // get a valid memory free callback for this context
    free_tree_fnc = T_valid_free_callback(
        free_tree_fnc,
        &mem_free_no_debug, // not allowed
        &D1_ignore // alternative to above
    );

    // traverse the tree in post order and free the tree nodes from the
    // bottom up.
    G = tree_generator_alloc(T, TREE_TRAVERSE_POSTORDER);

    while(generator_next(G)) {
        elm = generator_current(G);
        node = (Tree *)elm;

        if(0 < node->_degree)
            mem_free(node->_branches MEM_DEBUG_INFO);

        free_tree_fnc(elm);
        mem_free(elm MEM_DEBUG_INFO);
    }

    // free the generator
    generator_free(G);
    G = NULL;
    T = NULL;
}

/**
 * A tree free with no callback.
 */
void D1_tree_free(void *T) {
    tree_free(T, &D1_ignore);
}

/**
 * Return the number of branches allocated for this tree.
 */
size_t tree_degree(void *t) {
    if(NULL == t)
        return 0;
    return ((Tree *) t)->_degree;
}

/**
 * Return the number of allocated branches that have been filled.
 */
size_t tree_fill(void *t) {
    if(NULL == t)
        return 0;
    return ((Tree *) t)->_fill;
}

/**
 * Trim off all of the branches, and push each of them into the garbage stack.
 */
void tree_trim(void *t, Stack *S) {

    Tree *T = NULL, // tree
         *B = NULL; // branch
    size_t i;

    if(NULL == t || NULL == S)
        return;

    T = (Tree *) t;

    if(NULL == T->_branches || T->_degree == 0)
        return;

    // sub-trees are there, collect them into our garbage collection.
    if(T->_fill > 0) {
        for(i = 0; i < T->_fill; ++i) {
            B = (Tree *) (T->_branches[i]);
            stack_push(S, B);

            // update the branches parent information
            B->_parent_branch = 0;
            B->_parent = NULL;
            T->_branches[i] = NULL;
        }
    }

    mem_free(T->_branches MEM_DEBUG_INFO);

    // update the tree accordingly
    T->_branches = NULL;
    T->_degree = 0;
    T->_fill = 0;
}

/**
 * Set a tree C as one of the branches of T. Return 1 if successful, 0 on failure.
 */
int tree_add_branch(void *T, void *C) {
    if(NULL == T || NULL == C)
        return 0;

    Tree *parent = (Tree *) T,
         *child = (Tree *) C;

    if(parent->_fill >= parent->_degree)
        return 0;

    parent->_branches[parent->_fill] = child;
    ++parent->_fill;

    return 1;
}

FREE_GENERATOR_FUNC(T_generator_free_s, Stack, stack_free)
FREE_GENERATOR_FUNC(T_generator_free_q, Queue, queue_free)

/**
 * Generate the next tree element in a depth-first traversal of the tree.
 */
static void *T_generator_next_df(void *g) {
    Stack *S = NULL;
    Tree *curr = NULL;
    void *ret = NULL;
    size_t i;
    TreeGenerator *G = NULL;

    if(NULL == g)
        return NULL;

    G = (TreeGenerator *) g;
    S = (Stack *) G->_adt;

    while(!stack_empty(S)) {

        ret = stack_pop(S);
        curr = (Tree *)ret;

        if(NULL == curr)
            continue;

        // push the branches onto the stack
        for(i = curr->_fill; i > 0; ) {
            stack_push(S, curr->_branches[--i]);
        }

        return ret;

    };

    return NULL;
}

/**
 * Generate the next tree element in a breadth-first traversal of the tree.
 */
static void *T_generator_next_bf(void *g) {
    Queue *Q = NULL;
    Tree *curr = NULL;
    void *ret = NULL;
    size_t i;
    TreeGenerator *G = NULL;

    if(NULL == g)
        return NULL;

    G = (TreeGenerator *) g;
    Q = (Queue *) G->_adt;

    while(!queue_empty(Q)) {
        ret = queue_pop(Q);
        curr = (Tree *)ret;

        if(NULL == curr)
            continue;

        // push the branches onto the queue
        for(i = 0; i < curr->_fill; ++i) {
            queue_push(Q, curr->_branches[i]);
        }

        return ret;

    };

    return NULL;
}

/**
 * Generate the nodes of a post-order traversal of a tree one at a time.
 */
static void *T_generator_next_po(void *g) {
    Stack *S = NULL;
    Tree *curr = NULL,
         *top;
    void *ret = NULL;
    size_t i;
    TreeGenerator *G = NULL;

    if(NULL == g)
        return NULL;

    G = (TreeGenerator *) g;
    S = (Stack *) G->_adt;

    if(stack_empty(S))
        return NULL;

    while(!stack_empty(S)) {
        curr = stack_peek(S);

        // subtrees have been explored
        if(NULL == curr) {

            // get the tree to return
            stack_pop(S);
            ret = stack_pop(S);
            curr = (Tree *)ret;

            // we are looking at the last element that is a direct descendant
            // of the tree on the top of the stack, push NULL on the stack to
            // notify us to ignore this nodes children in future
            if(!stack_empty(S)) {

                top = (Tree *)stack_peek(S);

                if(NULL == top)
                    break;

                if(top->_fill == 0 || top->_branches[top->_fill - 1] == curr) {
                    stack_push(S, NULL);
                }
            }

            break;

        // subtrees weren't explored, explore them!
        } else {
            do {
                // add in the children of this node to the stack
                if(curr->_fill > 0) {

                    for(i = curr->_fill; i > 0; ) {
                        stack_push(S, curr->_branches[--i]);
                    }

                    curr = stack_peek(S);

                // no sub-trees to explore, this is a leaf node
                } else {
                    stack_push(S, NULL);
                    break;
                }

            } while(NULL != curr);
        }
    }

    return ret;
}

/**
 * Allocate a tree generator on the heap and initialize that generator.
 */
TreeGenerator *tree_generator_alloc(void *tree, const TreeTraversal traverse_type) {

    Tree *T = (Tree *) tree;
    TreeGenerator *G = NULL;

    if(NULL == T)
        return NULL;

    // allocate the generator
    G = generator_alloc(sizeof(TreeGenerator));
    G->_adt = NULL;

    // initialize the various types of generators
    switch(traverse_type) {

        // post-order traversal, doesn't use the generic ADT methods of the
        // generator
        case TREE_TRAVERSE_POSTORDER:
            G->_adt = stack_alloc(0);
            generator_init(G, &T_generator_next_po, &T_generator_free_s);
            stack_push(G->_adt, T);
            break;

        // depth-first traversal
        case TREE_TRAVERSE_PREORDER:
            G->_adt = stack_alloc(0);
            generator_init(G, &T_generator_next_df, &T_generator_free_s);
            stack_push(G->_adt, T);
            break;

        // breadth-first traversal
        case TREE_TRAVERSE_LEVELORDER:
            G->_adt = queue_alloc(0);
            generator_init(G, &T_generator_next_bf, &T_generator_free_q);
            queue_push(G->_adt, T);
            break;
    }

    return G;
}
