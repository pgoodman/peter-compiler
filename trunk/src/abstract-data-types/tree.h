/*
 * tree.h
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef TREE_H_
#define TREE_H_

/**
 * Threaded tree type.
 */
typedef struct Tree {
	size_t degree,
           fill;
	struct Tree *branches[];
} Tree;

typedef struct GenericTree {
    Tree;
    void *elm;
} GenericTree;

typedef struct TreeGenerator {
    Generator;

    void *adt;
    int (*adt_empty)(void *);
    D1 adt_push;
    F2 adt_pop;
    D2 adt_free;

    //TreeTraversal type;
    //Tree *tree;
} TreeGenerator;

typedef enum {
    TREE_TRAVERSE_INORDER,
    TREE_TRAVERSE_PREORDER,
    TREE_TRAVERSE_POSTORDER,
    TREE_TRAVERSE_LEVELORDER
} TreeTraversal;

/**
 * Tree visitor type.
 */
Tree *tree_alloc(int);
void tree_free(Tree *, D1);
void tree_free_visitor(Tree *);
void tree_traverse_df(Tree * const, D1);
void tree_traverse_bf(Tree * const, D1);

inline GenericTree *gen_tree_alloc(void);
void gen_tree_free(GenericTree *, D1);

#endif /* TREE_H_ */
