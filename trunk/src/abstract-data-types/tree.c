/*
 * tree.c
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "tree.h"
#include "stack.h"
#include "queue.h"
#include "delegate.h"
#include "function.h"
#include "generator.h"

/**
 * Allocate a new tree on the heap.
 */
Tree *tree_alloc(int size, const int degree) {
    Tree *T = NULL,
         *B[] = NULL;

    if(size < sizeof(Tree))
        size = sizeof(Tree);

    T = malloc(size);
    if(NULL == T)
        mem_error("Unable to allocate a tree on the heap.");

    B = malloc(size * degree);
    if(NULL == B)
        mem_error("Unable to allocate the branches for a tree on the heap.");

    T->degree = degree;
    T.branches = B;

    return T;
}

/**
 * Free the pointers of a tree. This takes in a visitor callback that can perform
 * any manual freeing on the tree.
 */
void tree_free(Tree * const tree, D1 free_visitor) {
    tree_traverse_bf(tree, free_visitor);
}

/**
 * Free the pointers that form the tree structure of a free node. This should
 * be called within any custom free visitor passed in to tree_free() by a coder.
 */
void tree_free_visitor(Tree *T) {
    free(T.branches);
    free(T);
    T = NULL;
}

/**
 * Generate the next tree element in a depth-first traversal of the tree.
 */
Tree *traverse_df_generate(TreeGenerator *G) {
    Stack *S = G->adt;
    Tree *curr = NULL;
    size_t i;

    while(!stack_empty(S)) {
        curr = stack_pop(S);

        if(NULL == curr)
            continue;

        // push the branches onto the stack
        for(i = curr->degree - 1; i >= 0; --i)
            stack_push(S, curr->branches[i]);

        return curr;

    } while(1);

    return NULL;
}

/**
 * Generate the next tree element in a breadth-first traversal of the tree.
 */
Tree *traverse_bf_generate(TreeGenerator *G) {
    Queue *Q = G->adt;
    Tree *curr = NULL;
    size_t i;

    while(!queue_empty(Q)) {
        curr = queue_pop(Q);

        if(NULL == curr)
            continue;

        // push the branches onto the queue
        for(i = curr->degree - 1; i >= 0; --i)
            queue_push(Q, curr->branches[i]);

        return curr;

    } while(1);

    return NULL;
}

/**
 * Generate the nodes of a post-order traversal of a tree one at a time.
 */
Tree *traverse_po_generate(TreeGenerator *G) {
    Stack *S = G->adt;
    Tree *curr = NULL,
         *top;
    size_t i;

    if(stack_empty(S))
        return NULL;

    while(!stack_empty(S)) {
        curr = stack_peek(S);

        // subtrees have been explored
        if(NULL == curr) {
            stack_pop(S);
            curr = stack_pop(S);

            // we are looking at the last element that is a direct descendent
            // of the tree on the top of the stack, push NULL on the stack to
            // notify us to ignore this nodes children in future
            if(!stack_empty(S)) {
                top = stack_peek(S);

                if(top->branches[top->degree-1] == curr) {
                    stack_push(S, NULL);
                }
            }

            break;
        // subtrees weren't explored, explore them!
        } else {
            do {
                // add in the children of this node to the stack
                if(curr->degree > 0) {
                    for(i = curr->degree - 1; i >= 0; --i)
                        stack_push(S, curr->branches[i]);

                    curr = stack_peek(S);

                // no sub-trees to explore, this is a leaf node
                } else {
                    stack_push(S, NULL);
                    break;
                }

            } while(1);
        }
    }

    return curr;
}

/**
 * Free a tree generator.
 */
void tree_generator_free(TreeGenerator *G) {
    if(NULL == G)
        return;

    stack_free((Stack *) G->adt, D1_ignore);
    G->adt = NULL;

    free(G);
    G = NULL;
}

/**
 * Allocate a tree generator on the stack and initialize that generator.
 */
TreeGenerator *tree_generator(Tree *T, const TreeTraversal type) {

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
            g->generate = &traverse_po_generate;
            g->free = &tree_generator_free_stack;
            stack_push(G->adt, T);
            break;

        // depth-first traversal
        case TREE_TRAVERSE_PREORDER:
            G->adt = stack_alloc();
            g->free = &tree_generator_free_stack;
            g->generate = &traverse_df_generate;
            stack_push(G->adt, T);
            break;

        // breadth-first traversal
        case TREE_TRAVERSE_LEVELORDER:
            G->adt = queue_alloc();
            g->free = &tree_generator_free;
            g->generate = &traverse_bf_generate;
            queue_push(G->adt, T);
            break;
    }

    return G;
}

inline GenericTree *gen_tree_alloc(void) {
    GenericTree *T = tree_alloc(sizeof(GenericTree));
    T->elm = NULL;
    return T;
}

void gen_tree_free(GenericTree * T, D1 free_elm) {

}
