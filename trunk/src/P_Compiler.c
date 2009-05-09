/*
 ============================================================================
 Name        : P_Compiler.c
 Author      : Peter Goodman
 Version     :
 ============================================================================
 */

#include <std-include.h>
#include <adt-stack.h>
#include <adt-tree.h>
#include <adt-vector.h>
#include <func-delegate.h>

typedef struct CharTree {
    Tree _;
    char letter;
} CharTree;

int main() { $MH

    size_t s = sizeof(CharTree);
    TreeGenerator *gen = NULL;
    Stack *S = NULL;

    printf("before alloc trees.\n");

    CharTree *A = tree_alloc(s, 3 _$$),
             *B = tree_alloc(s, 3 _$$),
             *C = tree_alloc(s, 0 _$$),
             *D = tree_alloc(s, 2 _$$),
             *E = tree_alloc(s, 0 _$$),
             *F = tree_alloc(s, 1 _$$),
             *G = tree_alloc(s, 0 _$$),
             *H = tree_alloc(s, 1 _$$),
             *I = tree_alloc(s, 0 _$$),
             *J = tree_alloc(s, 0 _$$),
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

    tree_add_branch(A, B _$$);
        tree_add_branch(B, E _$$);
        tree_add_branch(B, F _$$);
            tree_add_branch(F, H _$$);
        tree_add_branch(B, G _$$);
    tree_add_branch(A, C _$$);
    tree_add_branch(A, D _$$);
        tree_add_branch(D, I _$$);
        tree_add_branch(D, J _$$);

    /* try out the post-ordering generator */
    printf("Post-order:\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_POSTORDER _$$);
    while(generator_next(gen _$$)) {
        curr = generator_current(gen _$$);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen _$$);

    /* try out the pre-ordering generator */
    printf("Pre-order (depth-first):\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_PREORDER _$$);
    while(generator_next(gen _$$)) {
        curr = generator_current(gen _$$);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen _$$);

    /* try out the level ordering generator */
    printf("Level-order (breadth-first):\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_LEVELORDER _$$);
    while(generator_next(gen _$$)) {
        curr = generator_current(gen _$$);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen _$$);

    /* try out tree trimming the branches of and internal node */
    S = stack_alloc(0 _$$);
    tree_trim(B, S _$$);

    /* see what the post-order traversal looks like */
    printf("Post-order AFTER tree trimming the branches of B:\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_POSTORDER _$$);
    while(generator_next(gen _$$)) {
        curr = generator_current(gen _$$);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen _$$);

    printf("freeing resources..\n");

    stack_free(S, &D1_tree_free _$$);
    tree_free(A, &D1_ignore _$$);

    printf("resources freed, testing vector operations.\n");

    Vector *V = vector_alloc(10 _$$);
    vector_set(V, 0, tree_alloc(s, 4 _$$), D1_ignore _$$);
    vector_free(V, D1_tree_free _$$);

    printf("vector operations work.\n");

    return 0;
}
