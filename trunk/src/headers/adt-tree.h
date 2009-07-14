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
                   _fill; /* number of branches with children */

	struct PTree **_branches; /* array of branches */
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
} PTreeTraversalType;

/* tree operations */
void *tree_alloc(const size_t, const unsigned short );
void tree_free(PTree *, PDelegate);
void delegate_tree_free(PTree * );
void tree_clear(PTree *tree);
void tree_clear_num(PTree *tree, unsigned short num_branches);
void tree_trim(PTree *, PDictionary *);
void tree_add_branch(PTree *, PTree *);
void tree_force_add_branch(PTree *, PTree *);
void tree_force_add_branch_children(PTree *, PTree *);
unsigned short tree_get_num_slots(PTree * );
unsigned short tree_get_num_branches(PTree * );
void *tree_parent(PTree *);

/* tree generator */
PTreeGenerator *tree_generator_alloc(void *, const PTreeTraversalType );
PTreeGenerator *tree_generator_init(PTreeGenerator *, void *, const PTreeTraversalType );
void tree_generator_reuse(PTreeGenerator *, void *);

#define tree_get_branch(T,branch) (((PTree *) T)->_branches[branch])
#define tree_get_branches(T) (((PTree *) T)->_branches)

unsigned long int tree_num_allocated_pointers(void);

#endif /* TREE_H_ */
