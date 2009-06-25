/*
 * tree.c
 *
 *  Created on: May 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-tree.h>

/* -------------------------------------------------------------------------- */

static unsigned long int num_allocations = 0;

#define tree_mem_alloc(x) mem_alloc(x); ++num_allocations
#define tree_mem_calloc(x,y) mem_calloc(x,y); ++num_allocations
#define tree_mem_free(x) mem_free(x); --num_allocations
#define tree_mem_error(x) mem_error(x)

unsigned long int tree_num_allocated_pointers(void) {
    return num_allocations;
}

/* -------------------------------------------------------------------------- */

/**
 * Figure out a valid memory freeing callback for freeing things.
 */
static PDelegate *T_valid_free_callback(PDelegate *ft,
                                       PDelegate *disallowed,
                                       PDelegate *allowed) {
    PDelegate *free_tree = NULL;

    /* make sure we don't over free */
    assert_not_null(ft);

    if (disallowed == ft) {
        free_tree = allowed;
    }

    if (is_null(free_tree)) {
        free_tree = ft;
    }

    return free_tree;
}

/* Free a single tree node. */
static void T_free(PTree *tree, PDelegate *free_tree_fnc) {
    tree->_degree = 0;
    tree->_fill = 0;
    if(is_not_null(tree->_branches)) {
        tree_mem_free(tree->_branches);
    }
    free_tree_fnc(tree);
    tree_mem_free(tree);
    return;
}

/**
 * Set a tree 'branch' as one of the branches of 'parent'. Return 1 if
 * successful, 0 on failure.
 */
static void T_add_branch(PTree *parent, PTree *branch, int force) {
    PTree *child = (PTree *) branch;

    assert_not_null(parent);
    assert_not_null(branch);
    assert(parent->_fill < parent->_degree || force);

    if(parent->_fill >= parent->_degree && force) {
        parent->_degree = (0 == parent->_degree) ? 1 : parent->_degree * 2;

        parent->_branches = mem_realloc(
            parent->_branches,
            (parent->_degree * sizeof(PTree *))
        );

        if(is_null(parent->_branches)) {
            mem_error("Unable to grow parse tree node.");
        }
    }

    parent->_branches[parent->_fill] = child;

    ++(parent->_fill);
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a new N-ary tree on the heap. The struct_size is the size of the
 * structure which *must* contain a PTree as its first field, and the degree
 * is the number of branches that this tree should have.
 */
void *tree_alloc(const size_t struct_size, const unsigned short degree) {
    PTree *T = NULL;

    assert(sizeof(PTree) <= struct_size);

    T = tree_mem_alloc(struct_size);
    T->_branches = NULL;
    T->_degree = degree;
    T->_fill = 0;

    if (degree > 0) {
        T->_branches = tree_mem_alloc(sizeof(PTree *) * degree);
    }

    return T;
}

/**
 * Free the pointers of a tree. This takes in a visitor call-back that can
 * perform any manual freeing on the tree.
 *
 * !!! The call-back is responsible ONLY for freeing any heap allocated fields
 *     which are not part of the PTree structure. The PTree structure should be
 *     seen as a black box and not touched.
 */
void tree_free(PTree *T, PDelegate *free_tree_fnc) {
    PTreeGenerator *G = NULL;

    assert_not_null(T);
    assert_not_null(free_tree_fnc);

    /* get a valid memory free callback for this context */
    free_tree_fnc = T_valid_free_callback(
        free_tree_fnc,
        &delegate_mem_free, /* not allowed */
        &delegate_do_nothing /* alternative to above */
    );

    /* traverse the tree in post order and free the tree nodes from the
     * bottom up. */
    if (T->_fill > 0) {
        G = tree_generator_alloc(T, TREE_TRAVERSE_POSTORDER);
        while (generator_next(G)) {
            T_free(generator_current(G), free_tree_fnc);
        }
        generator_free(G);

    /* task is simple, just free the tree. */
    } else {
        T_free(T, free_tree_fnc);
    }

    return;
}

/**
 * A tree free with no callback.
 */
void delegate_tree_free(PTree *T) {
    tree_free(T, &delegate_do_nothing);
    return;
}

/**
 * Return the number of branches allocated for this tree.
 */
unsigned short tree_get_num_slots(PTree *T) {
    assert_not_null(T);
    return T->_degree;
}

/**
 * Return the number of allocated branches that have been filled.
 */
unsigned short tree_get_num_branches(PTree *T) {
    assert_not_null(T);
    return T->_fill;
}

/**
 * Clear off all of the branches, without doing any proper cleaning up on them.
 */
void tree_clear(PTree *tree) {
    int i = 0;

    assert_not_null(tree);
    tree->_fill = 0;

    return;
}

/**
 * Clear a certain number of branches from a tree.
 */
void tree_clear_num(PTree *tree, unsigned short num_branches) {
    assert_not_null(tree);
    assert(tree->_fill >= num_branches);
    tree->_fill -= num_branches;
}

/**
 * Trim off all of the branches, and push each of them into the garbage stack.
 */
void tree_trim(PTree *T, PDictionary *garbage_set) {
    PTree *branch = NULL;
    unsigned short i;

    assert_not_null(T);
    assert_not_null(garbage_set);

    /* none of the branches have been filled, or no branches were allocated,
     * so we have nothing to do.. yay! */
    if (T->_fill == 0 || is_null(T->_branches)) {
        return;
    }

    /* sub-trees are there, collect them into our garbage collection. */
    for (i = 0; i < T->_fill; ++i) {
        branch = (PTree *) (T->_branches[i]);
        dict_set(garbage_set, branch, branch, &delegate_do_nothing);

        /* update the branches parent information */
        T->_branches[i] = NULL;
    }

    /* update the tree accordingly */
    T->_fill = 0;

    return;
}

void tree_add_branch(PTree *parent, PTree *branch) {
    T_add_branch(parent, branch, 0);
}

void tree_force_add_branch(PTree *parent, PTree *branch) {
    T_add_branch(parent, branch, 1);
}

void tree_force_add_branch_children(PTree *parent, PTree *branch) {
    unsigned short i = 0,
                   fill;

    assert_not_null(parent);
    assert_not_null(branch);

    if(0 == branch->_fill) {
        return;
    }

    fill = branch->_fill;
    for(; i < fill; ++i) {
        T_add_branch(parent, branch->_branches[i], 1);
    }
}

/* -------------------------------------------------------------------------- */

#define FREE_GENERATOR_FUNC(N, T, F) \
    static void N(void *g ) {\
        PTreeGenerator *G = NULL; \
        T *A = NULL; \
        assert_not_null(g); \
        G = (PTreeGenerator *) g; \
        A = (T *) (G->_adt); \
        G->_adt = NULL; \
        F(A, delegate_do_nothing ); \
        tree_mem_free(g); \
        G = NULL; \
        g = NULL; \
        return; \
    }

FREE_GENERATOR_FUNC(T_generator_free_s, PStack, stack_free)
FREE_GENERATOR_FUNC(T_generator_free_q, PQueue, queue_free)

/**
 * Generate the next tree element in a depth-first traversal of the tree.
 */
static void *T_generator_next_df(PTreeGenerator *G) {
    PStack *S = NULL;
    PTree *curr = NULL;
    void *ret = NULL;
    unsigned short i;

    assert_not_null(G);

    S = (PStack *) G->_adt;

    while (!stack_is_empty(S)) {

        ret = stack_pop(S);
        curr = (PTree *) ret;

        if (is_null(curr)) {
            continue;
        }

        /* push the branches onto the stack */
        for (i = curr->_fill; i > 0;) {
            stack_push(S, curr->_branches[--i]);
        }

        return ret;
    }
    return NULL;
}

/**
 * Generate the next tree element in a breadth-first traversal of the tree.
 */
static void *T_generator_next_bf(PTreeGenerator *G) {
    PQueue *Q = NULL;
    PTree *curr = NULL;
    void *ret = NULL;
    unsigned short i;

    assert_not_null(G);

    Q = (PQueue *) G->_adt;

    while (!queue_is_empty(Q)) {
        ret = queue_pop(Q);
        curr = (PTree *) ret;

        if (is_null(curr)) {
            continue;
        }

        /* push the branches onto the queue */
        for (i = 0; i < curr->_fill; ++i) {
            queue_push(Q, curr->_branches[i]);
        }

        return ret;
    }
    return NULL;
}

/**
 * Generate the nodes of a post-order traversal of a tree one at a time.
 */
static void *T_generator_next_po(PTreeGenerator *G) {
    PStack *S = NULL;
    PTree *curr = NULL, *top = NULL;
    void *ret = NULL;
    unsigned short i;

    assert_not_null(G);

    S = (PStack *) G->_adt;

    if (stack_is_empty(S)) {
        return NULL;
    }

    while (!stack_is_empty(S)) {

        curr = stack_peek(S);

        /* subtrees have been explored */
        if (NULL == curr) {

            /* get the tree to return */
            stack_pop(S);
            ret = stack_pop(S);
            curr = (PTree *) ret;

            /* we are looking at the last element that is a direct descendant
             * of the tree on the top of the stack, push NULL on the stack to
             * notify us to ignore this nodes children in future. */
            if (!stack_is_empty(S)) {

                top = (PTree *) stack_peek(S);

                if (NULL == top)
                    break;

                if (top->_fill == 0 || top->_branches[top->_fill - 1] == curr) {
                    stack_push(S, NULL);
                }
            }

            break;

            /* subtrees weren't explored, explore them! */
        } else {
            do {

                /* add in the children of this node to the stack */
                if (curr->_fill > 0) {
                    for (i = curr->_fill; i > 0;) {
                        stack_push(S, curr->_branches[--i]);
                    }

                    curr = stack_peek(S);

                /* no sub-trees to explore, this is a leaf node */
                } else {
                    stack_push(S, NULL);
                    break;
                }

            } while (NULL != curr);
        }
    }

    return ret;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a tree generator on the heap and initialize that generator.
 */
PTreeGenerator *tree_generator_alloc(void *tree,
                                     const PTreeTraversal traverse_type) {
    PTree *T;
    PTreeGenerator *gen;

    assert_not_null(tree);

    T = (PTree *) tree;
    gen = generator_alloc(sizeof(PTreeGenerator));
    gen->_adt = NULL;

    /* initialize the various types of generators */
    switch (traverse_type) {

    /* post-order traversal, doesn't use the generic ADT methods of the
     * generator */
    case TREE_TRAVERSE_POSTORDER:
        gen->_adt = stack_alloc(sizeof(PStack));
        gen->_reclaim_adt = (PTreeGeneratorReclaimFunction) &stack_empty;
        generator_init(
            gen,
            (PFunction *) &T_generator_next_po,
            (PDelegate *) &T_generator_free_s
        );
        stack_push(gen->_adt, T);
        break;

        /* depth-first traversal */
    case TREE_TRAVERSE_PREORDER:
        gen->_adt = stack_alloc(sizeof(PStack));
        gen->_reclaim_adt = (PTreeGeneratorReclaimFunction) &stack_empty;
        generator_init(
            gen,
            (PFunction *) &T_generator_next_df,
            (PDelegate *) &T_generator_free_s
        );
        stack_push(gen->_adt, T);
        break;

        /* breadth-first traversal */
    case TREE_TRAVERSE_LEVELORDER:
        gen->_adt = queue_alloc(sizeof(PQueue));
        gen->_reclaim_adt = (PTreeGeneratorReclaimFunction) &queue_empty;
        generator_init(
            gen,
            (PFunction *) &T_generator_next_bf,
            (PDelegate *) &T_generator_free_q
        );
        queue_push(gen->_adt, T);
        break;
    }

    return gen;
}

/**
 * Reuse a tree generator on a new data structure.
 */
void tree_generator_reuse(PTreeGenerator *G, void *tree) {
    assert_not_null(G);
    assert_not_null(tree);

    G->_reclaim_adt(G->_adt, &delegate_do_nothing);
    G->_adt = tree;
}
