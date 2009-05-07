/*
 * main.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include "abstract-data-types/stack.h"
#include "abstract-data-types/tree.h"

typedef struct CharTree {
    Tree _;
    char letter;
} CharTree;

int main() {
    int s = sizeof(CharTree);
    TreeGenerator *gen = NULL;
    Stack *S = NULL;

    printf("before alloc trees.\n");

    CharTree *A = tree_alloc(s, 3),
             *B = tree_alloc(s, 3),
             *C = tree_alloc(s, 0),
             *D = tree_alloc(s, 2),
             *E = tree_alloc(s, 0),
             *F = tree_alloc(s, 1),
             *G = tree_alloc(s, 0),
             *H = tree_alloc(s, 1),
             *I = tree_alloc(s, 0),
             *J = tree_alloc(s, 0),
             *curr = NULL;

    A->letter = 'A';
    B->letter = 'B';
    C->letter = 'C';
    D->letter = 'D';
    E->letter = 'E';
    F->letter = 'F';
    G->letter = 'G';
    H->letter = 'H';
    I->letter = 'I';
    J->letter = 'J';

    tree_add_branch(A, B);
        tree_add_branch(B, E);
        tree_add_branch(B, F);
            tree_add_branch(F, H);
        tree_add_branch(B, G);
    tree_add_branch(A, C);
    tree_add_branch(A, D);
        tree_add_branch(D, I);
        tree_add_branch(D, J);

    // try out the post-ordering generator
    printf("Post-order:\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_POSTORDER);
    while(generator_next(gen)) {
        curr = generator_current(gen);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen);

    // try out the pre-ordering generator
    printf("Pre-order (depth-first):\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_PREORDER);
    while(generator_next(gen)) {
        curr = generator_current(gen);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen);

    // try out the level ordering generator
    printf("Level-order (breadth-first):\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_LEVELORDER);
    while(generator_next(gen)) {
        curr = generator_current(gen);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen);

    // try out tree trimming the branches of and internal node
    S = stack_alloc(0);
    tree_trim(B, S);

    // see what the post-order traversal looks like
    printf("Post-order AFTER tree trimming the branches of B:\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_POSTORDER);
    while(generator_next(gen)) {
        curr = generator_current(gen);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen);

    printf("freeing resources..\n");

    stack_free(S, &D1_tree_free);
    tree_free(A, &D1_ignore);

    printf("resources freed.\n");

    return 0;
}
