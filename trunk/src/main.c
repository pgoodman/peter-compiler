/*
 * main.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include "abstract-data-types/tree.h"

typedef struct CharTree {
    Tree _;
    char letter;
} CharTree;

int main() {
    int s = sizeof(CharTree);
    TreeGenerator *gen = NULL;

    CharTree *A = tree_alloc(s, 2),
             *B = tree_alloc(s, 2),
             *C = tree_alloc(s, 2),
             *D = tree_alloc(s, 2),
             *E = tree_alloc(s, 2),
             *F = tree_alloc(s, 2),
             *G = tree_alloc(s, 2),
             *H = tree_alloc(s, 2),
             *I = tree_alloc(s, 2),
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

    tree_add_branch(F, B); // root
    tree_add_branch(F, G);
    tree_add_branch(B, A);
    tree_add_branch(B, D);
    tree_add_branch(D, C);
    tree_add_branch(D, E);
    tree_add_branch(G, I);
    tree_add_branch(I, H);

    gen = tree_generator_alloc(F, TREE_TRAVERSE_PREORDER);
    while(NULL != (curr = generator_next(gen))) {
        printf(&(curr->letter));
    }

    return 0;
}
