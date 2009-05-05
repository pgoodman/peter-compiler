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

#if 0

/**
 * Singly-linked list data structure used internally to map trees.
 */
struct tree_list {
    Tree * const tree;
    struct tree_list * next;
};

/**
 * Free a list of trees.
 */
static void free_tree_list(struct tree_list *list) {
    struct tree_list *curr;
    while(NULL != list) {
        curr = list;
        list = curr->next;
        curr->tree = NULL;
        free(curr);
    }
    list = NULL;
}

/**
 * Use an already allocated but unused tree list or allocate a new one.
 */
static struct tree_list *alloc_tree_list(struct tree_list *unused) {
    struct tree_list *curr = NULL;
    if(NULL == unused) {
        curr = malloc(sizeof(struct tree_list));
        if(NULL == curr)
            exit(1); // TODO
    } else {
        curr = unused;
        unused = curr->next;
    }
    return curr;
}

#endif

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
 * A generic function to traverse an tree using either depth-first or
 * breadth-first traversal.
 */
/*static void generic_traverse(Tree * const T, D1 visit,
                            D1 adt_alloc, int (*adt_empty)(void *),
                            D1 adt_push, F2 adt_pop,
                            D2 adt_free) {

    void *ADT = adt_alloc();
    Tree *curr = NULL;
    size_t i;

    if(NULL == T || NULL == visit)
        return;

    adt_push(ADT, T);

    while(!adt_empty(ADT)) {
        curr = adt_pop(ADT);

        if(NULL == curr)
            continue;

        visit(curr);

        // push the branches onto the stack
        for(i = curr->degree - 1; i >= 0; --i)
            adt_push(ADT, curr->branches[i]);
    }

    adt_free(ADT, &D1_ignore);
}*/

/**
 * Perform a single step in getting the next tree item using either a depth-
 * first or breadth-first traveral of a tree.
 */
Tree *traverse_df_bf_generate(TreeGenerator *G) {
    void *ADT = G->adt;
    Tree *curr = NULL;

    while(!G->adt_empty(ADT)) {
        curr = G->adt_pop(ADT);

        if(NULL == curr)
            continue;

        // push the branches onto the stack/queue
        for(i = curr->degree - 1; i >= 0; --i)
            G->adt_push(ADT, curr->branches[i]);

        return curr;

    } while(1);

    return NULL;
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
    g = (Generator *)G;
    //G->tree = T;
    //G->type = type;

    // initialize the various types of generators
    switch(type) {
        case TREE_TRAVERSE_INORDER:

            break;

        // depth-first traversal
        case TREE_TRAVERSE_PREORDER:
            G->adt = stack_alloc();
            G->adt_empty = &stack_empty;
            G->adt_pop = &stack_pop;
            G->adt_push = &stack_push;
            g->free = &stack_free;
            g->generate = &traverse_df_bf_generate;

            stack_push(G->adt, T);
            break;

        // start at the leaves and work up from there, level-by-level
        case TREE_TRAVERSE_POSTORDER:

            break;

        // breadth-first traversal
        case TREE_TRAVERSE_LEVELORDER:
            G->adt = queue_alloc();
            G->adt_empty = &queue_empty;
            G->adt_pop = &queue_pop;
            G->adt_push = &queue_push;
            g->free = &queue_free;
            g->generate = &traverse_df_bf_generate;

            queue_push(G->adt, T);
            break;
    }

    return G;
}

#if 0
static void tree_generator_po(TreeGenerator *G) {

}

/**
 * Perform a top-down depth-first traversal of a tree.
 */
void tree_traverse_preorder(Tree * const T, D1 visit) {
    generic_traverse(T, visit,
        &stack_alloc,
        &stack_empty,
        &stack_push,
        &stack_pop,
        &stack_free
    );
}

/**
 * Perform a top-down breadth-first traversal of a tree.
 */
void tree_traverse_levelorder(Tree * const T, D1 visit) {
    generic_traverse(T, visit,
        &queue_alloc,
        &queue_empty,
        &queue_push,
        &queue_pop,
        &queue_free
    );
}

/**
 * Perform a bottom-up breadth-first traversal of a tree.
 */
void tree_traverse_postorder(Tree * const T, D1 visit) {
    size_t i;
    Tree *curr = NULL;
    Stack *S = stack_alloc();
    //Queue *Q = queue_alloc();

    stack_push(T);

    while(!stack_empty(S)) {
        curr = stack_peek(S);

        if(NULL == curr)
            break;

        if(curr->degree > 0) {

        } else {

        }
    }
}

/**
 * Perform an in-order traversal of a tree.
 */
void tree_traverse_inorder(Tree * const T, D1 visit) {

}
#endif

#if 0
/**
 * Perform a depth-first mapping of a tree. The visit function is called for
 * each tree node.
 */
void tree_map_df(Tree * const tree, D1 visit) {
    Stack *S = stack_alloc();
    Tree *curr = NULL;
    size_t i;

    if(NULL == tree || NULL == visit)
	    return;

    stack_push(S, tree);

    while(!stack_empty(S)) {

        // pop the node off and extract the tree node
        curr = stack_pop(S);

        visit(curr);

        // push the branches onto the stack
        for(i = 0; i < curr.degree; ++i)
            stack_push(S, curr->branches[i]);
    }

    stack_free(S, &D1_ignore);
}

/**
 * Perform a breadth-first mapping of a tree. This visit function is called for
 * each tree node.
 */
void tree_map_bf(Tree * const tree, tree_visitor visit) {
    Tree *curr;

    int i,
        d;

    Queue *Q;

    if(NULL == tree || NULL == visit)
        return;

    // enqueue the tree
    Q = queue_alloc();
    queue_push(tree);

    while(!queue_empty(Q)) {

        // dequeue the list element and extract the tree node
        curr = queue_pop(Q);

        if(NULL == curr)
            continue;

        visit(curr);

        // enqueue the children and re-use the list item
        for(i = 0; i < curr->degree; ++i)
            queue_push(Q, curr->branches[i]);
    }

    // free the queue and unused items
    free_tree_list(queue);
    free_tree_list(unused);
}
#endif

inline GenericTree *gen_tree_alloc(void) {
    GenericTree *T = tree_alloc(sizeof(GenericTree));
    T->elm = NULL;
    return T;
}

void gen_tree_free(GenericTree * T, D1 free_elm) {

}
