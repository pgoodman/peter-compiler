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
#include "mem.h"
#include "stack.h"
#include "queue.h"
#include "delegate.h"
#include "function.h"
#include "generator.h"

/**
 * Threaded tree type.
 */
typedef struct Tree {
	size_t _degree,
           _fill;
	struct Tree **_branches;
} Tree;

/*
typedef struct GenericTree {
    Tree;
    void *elm;
} GenericTree;
*/

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
int tree_add_branch(void *, void *);
size_t tree_degree(void *);
size_t tree_fill(void *);

// tree generator
TreeGenerator *tree_generator_alloc(void *, const TreeTraversal);
void T_generator_free(void *);
void *T_generator_next_df(void *);
void *T_generator_next_bf(void *);
void *T_generator_next_po(void *);

#endif /* TREE_H_ */
