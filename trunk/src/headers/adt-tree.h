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
#include "adt-dict.h"
#include "adt-queue.h"
#include "adt-generator.h"

/**
 * Threaded tree type.
 */
typedef struct PTree {
	unsigned short _degree, /* number of allocated branches */
                   _fill, /* number of branches with children */
                   _parent_branch; /* which branch of the parent this tree is using */

	struct PTree **_branches, /* array of branches */
                 *_parent; /* parent tree */
} PTree;

typedef void (*PTreeGeneratorReclaimFunction)(void *, PDelegate);

typedef struct PTreeGenerator {
    PGenerator _;
    void *_adt;
    PTreeGeneratorReclaimFunction _reclaim_adt;
} PTreeGenerator;

/* in-order is not well-defined for N-ary trees, hence its exclusion */
typedef enum {
    TREE_TRAVERSE_PREORDER,
    TREE_TRAVERSE_POSTORDER,
    TREE_TRAVERSE_LEVELORDER
} PTreeTraversal;

/* tree operations */
void *tree_alloc(const size_t, const unsigned short );
void tree_free(void *, PDelegate);
void delegate_tree_free(void * );
void tree_clear(void *tree, int do_clear);
void tree_trim(void *, PDictionary *);
void tree_trim_free(void *tree, PDelegate free_tree_fnc);
char tree_add_branch(void *, void * );
size_t tree_degree(void * );
size_t tree_fill(void * );
void *tree_parent(void *);
/*void *tree_get_branch(void *, unsigned short);*/

/* tree generator */
PTreeGenerator *tree_generator_alloc(void *, const PTreeTraversal );
PTreeGenerator *tree_generator_init(PTreeGenerator *, void *, const PTreeTraversal );
void tree_generator_reuse(PTreeGenerator *, void *);

#define tree_get_branch(T,branch) (((PTree *) T)->_branches[branch])

#endif /* TREE_H_ */
