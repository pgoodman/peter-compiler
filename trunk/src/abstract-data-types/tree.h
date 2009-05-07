/*
 * tree.h
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef TREE_H_
#define TREE_H_

#include <stdlib.h>
#include <memory-management/mem.h>
#include "stack.h"
#include "queue.h"
#include "delegate.h"
#include "function.h"
#include "generator.h"

/**
 * Threaded tree type.
 */
typedef struct Tree {
	size_t _degree, // number of allocated branches
           _fill, // number of branches with children
           _parent_branch; // which branch of the parent this tree is using

	struct Tree **_branches, // array of branches
                *_parent; // parent tree
} Tree;

typedef struct TreeGenerator {
    Generator _;
    void *_adt;
} TreeGenerator;

// in-order is not well-defined for N-ary trees, hence its exclusion
typedef enum {
    TREE_TRAVERSE_PREORDER,
    TREE_TRAVERSE_POSTORDER,
    TREE_TRAVERSE_LEVELORDER
} TreeTraversal;

// tree operations
void *tree_alloc(int, const size_t);
void tree_free(void *, D1);
void D1_tree_free(void *);
void tree_trim(void *, Stack *);
int tree_add_branch(void *, void *);
size_t tree_degree(void *);
size_t tree_fill(void *);

// tree generator
TreeGenerator *tree_generator_alloc(void *, const TreeTraversal);

#endif /* TREE_H_ */
