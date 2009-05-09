/*
 * tree.h
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef TREE_H_
#define TREE_H_

#include "std-include.h"
#include "func-delegate.h"
#include "func-function.h"
#include "adt-stack.h"
#include "adt-queue.h"
#include "adt-generator.h"

/**
 * Threaded tree type.
 */
typedef struct Tree {
	short _degree, /* number of allocated branches */
          _fill, /* number of branches with children */
          _parent_branch; /* which branch of the parent this tree is using */

	struct Tree **_branches, /* array of branches */
                *_parent; /* parent tree */
} Tree;

typedef struct TreeGenerator {
    Generator _;
    void *_adt;
} TreeGenerator;

/* in-order is not well-defined for N-ary trees, hence its exclusion */
typedef enum {
    TREE_TRAVERSE_PREORDER,
    TREE_TRAVERSE_POSTORDER,
    TREE_TRAVERSE_LEVELORDER
} TreeTraversal;

/* tree operations */
void *tree_alloc(size_t, const unsigned short $$);
void tree_free(void *, D1_t $$);
void D1_tree_free(void * $$);
void tree_trim(void *, Stack * $$);
char tree_add_branch(void *, void * $$);
size_t tree_degree(void * $$);
size_t tree_fill(void * $$);

/* tree generator */
TreeGenerator *tree_generator_alloc(void *, const TreeTraversal $$);
TreeGenerator *tree_generator_init(TreeGenerator *, void *, const TreeTraversal $$);

#endif /* TREE_H_ */
