/*
 * tree.c
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "tree.h"

/**
 * Allocate a new tree on the heap.
 */
void *tree_alloc(int size, const size_t degree) {
    void *tree = NULL;
    Tree *T = NULL,
         **B = NULL;

    if(size < sizeof(Tree))
        size = sizeof(Tree);

    tree = mem_alloc(size MEM_DEBUG_INFO);
    if(NULL == tree)
        mem_error("Unable to allocate a tree on the heap.");

    T = (Tree *)tree;
    B = mem_alloc(size * degree MEM_DEBUG_INFO);
    if(NULL == B)
        mem_error("Unable to allocate the branches for a tree on the heap.");

    T->degree = degree;
    T->fill = 0;
    T->branches = B;

    return tree;
}

/**
 * Free the pointers of a tree. This takes in a visitor call-back that can
 * perform any manual freeing on the tree.
 *
 * !!! The call-back is responsible ONLY for freeing any heap allocated fields
 *     which are not part of the Tree structure. The Tree structure should be
 *     seen as a black box and not touched.
 */
void tree_free(void *T, D1 free_tree_visitor) {

    TreeGenerator *G = NULL;
    void *elm = NULL;

    if(NULL == T)
        return;

    if(NULL == free_tree_visitor)
        free_tree_visitor = &D1_ignore;

    // traverse the tree in post order and free the tree nodes from the bottom up.
    G = tree_generator_alloc(T, TREE_TRAVERSE_POSTORDER);
    while(NULL != (elm = generator_next(G))) {
        free_tree_visitor(elm);
        mem_free(((Tree *) elm)->branches MEM_DEBUG_INFO);
        mem_free(elm MEM_DEBUG_INFO);
    }

    // free the generator
    generator_free(G);
    G = NULL;
    T = NULL;
}

/**
 * Set a tree C as one of the branches of T. Return 1 if successful, 0 on failure.
 */
int tree_add_branch(void *T, void *C) {
    if(NULL == T || NULL == C)
        return 0;

    Tree *parent = (Tree *) T,
         *child = (Tree *) C;

    if(parent->fill >= parent->degree)
        return 0;

    parent->branches[parent->fill] = child;
    ++parent->fill;

    return 1;
}

/**
 * Allocate a tree generator on the stack and initialize that generator.
 */
TreeGenerator *tree_generator_alloc(void *tree, const TreeTraversal type) {

    Tree *T = (Tree *)tree;
    TreeGenerator *G = NULL;
    Generator *g = NULL; // the internal generator within G

    if(NULL == T)
        return NULL;

    // allocate the generator
    G = generator_alloc(sizeof(TreeGenerator));
    G->adt = NULL;

    g = (Generator *)G;

    // initialize the various types of generators
    switch(type) {

        // post-order traversal, doesn't use the generic adt methods of the
        // generator
        case TREE_TRAVERSE_POSTORDER:
            G->adt = stack_alloc();
            g->_free = &T_generator_free;
            g->_gen = &T_traverse_po_generate;
            stack_push(G->adt, T);
            break;

        // depth-first traversal
        case TREE_TRAVERSE_PREORDER:
            G->adt = stack_alloc();
            g->_free = &T_generator_free;
            g->_gen = &T_traverse_df_generate;
            stack_push(G->adt, T);
            break;

        // breadth-first traversal
        case TREE_TRAVERSE_LEVELORDER:
            G->adt = queue_alloc();
            g->_free = &T_generator_free;
            g->_gen = &T_traverse_bf_generate;
            queue_push(G->adt, T);
            break;
    }

    return G;
}

/**
 * Free a tree generator.
 */
void T_generator_free(void *g) {
    TreeGenerator *G = NULL;

    if(NULL == g)
        return;

    G = (TreeGenerator *) g;
    printf("freeing a stack of a generator.\n");
    stack_free((Stack *) (G->adt), D1_ignore);
    G->adt = NULL;

    mem_free(g MEM_DEBUG_INFO);
    g = NULL;
    G = NULL;
}

/**
 * Generate the next tree element in a depth-first traversal of the tree.
 */
void *T_traverse_df_generate(void *g) {
    Stack *S = NULL;
    Tree *curr = NULL;
    void *ret = NULL;
    size_t i;
    TreeGenerator *G = NULL;

    if(NULL == g)
        return NULL;

    G = (TreeGenerator *) g;
    S = (Stack *) G->adt;

    while(!stack_empty(S)) {

        ret = stack_pop(S);
        curr = (Tree *)ret;

        if(NULL == curr)
            continue;

        // push the branches onto the stack
        for(i = curr->fill; i > 0; ) {
            stack_push(S, curr->branches[--i]);
        }

        return ret;

    };

    return NULL;
}

/**
 * Generate the next tree element in a breadth-first traversal of the tree.
 */
void *T_traverse_bf_generate(void *g) {
    Queue *Q = NULL;
    Tree *curr = NULL;
    void *ret = NULL;
    size_t i;
    TreeGenerator *G = NULL;

    if(NULL == g)
        return NULL;

    G = (TreeGenerator *) g;
    Q = (Queue *) G->adt;

    while(!queue_empty(Q)) {

        ret = queue_pop(Q);
        curr = (Tree *)ret;

        if(NULL == curr)
            continue;

        // push the branches onto the queue
        for(i = 0; i < curr->fill; ++i) {
            queue_push(Q, curr->branches[i]);
        }

        return ret;

    };

    return NULL;
}

/**
 * Generate the nodes of a post-order traversal of a tree one at a time.
 */
void *T_traverse_po_generate(void *g) {
    Stack *S = NULL;
    Tree *curr = NULL,
         *top;
    void *ret = NULL;
    size_t i;
    TreeGenerator *G = NULL;

    if(NULL == g)
        return NULL;

    G = (TreeGenerator *) g;
    S = (Stack *) G->adt;

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

                if(top->fill == 0 || top->branches[top->fill - 1] == curr) {
                    stack_push(S, NULL);
                }
            }

            break;

        // subtrees weren't explored, explore them!
        } else {
            do {
                // add in the children of this node to the stack
                if(curr->fill > 0) {

                    for(i = curr->fill; i > 0; ) {
                        stack_push(S, curr->branches[--i]);
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
