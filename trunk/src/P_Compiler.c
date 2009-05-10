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
#include <adt-hash-table.h>
#include <func-delegate.h>

typedef struct CharPTree {
    PTree _;
    char letter;
} CharTree;

unsigned int __st_depth = 0;

int main() { $MH

    size_t s = sizeof(CharTree);
    PTreeGenerator *gen = NULL;
    PStack *S = NULL;

    printf("before alloc trees.\n");

    CharTree *A = tree_alloc(s, 3 $$A),
             *B = tree_alloc(s, 3 $$A),
             *C = tree_alloc(s, 0 $$A),
             *D = tree_alloc(s, 2 $$A),
             *E = tree_alloc(s, 0 $$A),
             *F = tree_alloc(s, 1 $$A),
             *G = tree_alloc(s, 0 $$A),
             *H = tree_alloc(s, 1 $$A),
             *I = tree_alloc(s, 0 $$A),
             *J = tree_alloc(s, 0 $$A),
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

    tree_add_branch(A, B $$A);
        tree_add_branch(B, E $$A);
        tree_add_branch(B, F $$A);
            tree_add_branch(F, H $$A);
        tree_add_branch(B, G $$A);
    tree_add_branch(A, C $$A);
    tree_add_branch(A, D $$A);
        tree_add_branch(D, I $$A);
        tree_add_branch(D, J $$A);

    /* try out the post-ordering generator */
    printf("Post-order:\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_POSTORDER $$A);
    while(generator_next(gen $$A)) {
        curr = generator_current(gen $$A);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen $$A);

    /* try out the pre-ordering generator */
    printf("Pre-order (depth-first):\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_PREORDER $$A);
    while(generator_next(gen $$A)) {
        curr = generator_current(gen $$A);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen $$A);

    /* try out the level ordering generator */
    printf("Level-order (breadth-first):\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_LEVELORDER $$A);
    while(generator_next(gen $$A)) {
        curr = generator_current(gen $$A);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen $$A);

    /* try out tree trimming the branches of and internal node */
    S = stack_alloc(sizeof(PStack) $$A);
    tree_trim(B, S $$A);

    /* see what the post-order traversal looks like */
    printf("Post-order AFTER tree trimming the branches of B:\n");
    gen = tree_generator_alloc(A, TREE_TRAVERSE_POSTORDER $$A);
    while(generator_next(gen $$A)) {
        curr = generator_current(gen $$A);
        printf("\t%c\n", curr->letter);
    }
    generator_free(gen $$A);

    printf("freeing resources..\n");

    stack_free(S, &PDelegateree_free $$A);
    tree_free(A, &delegate_do_nothing $$A);

    printf("resources freed, testing vector operations.\n");

    PVector *V = vector_alloc(10 $$A);
    vector_set(V, 0, tree_alloc(s, 4 $$A), delegate_do_nothing $$A);
    vector_free(V, PDelegateree_free $$A);

    printf("vector operations work.\n");
    printf("trying hash table operations.\n");

    PHashTable *table = hash_table_alloc(100, &hash_table_hash_pointer $$A);
    int a = 10;

    printf("adding/getting 100 hash table elements.\n");

    for(; a < 100; ++a) {
        hash_table_set(table, (void *) a, (void *) a, &delegate_do_nothing $$A);
        hash_table_get(table, (void *) a $$A);
    }

    printf("removing all 100 elements.\n");

    for(a = 10; a < 100; ++a) {
        hash_table_unset(table, (void *) a, &delegate_do_nothing $$A);
    }

    printf("freeeing table.\n");

    hash_table_free(table, &delegate_do_nothing $$A);

    printf("table freed.\n");

    printf("testing string stuff.\n");

    PString *X = string_alloc_char("hello world", 11 $$A),
            *Y = string_alloc_char("hello world", 11 $$A);

    printf("are these strings equal? %s\n", string_equal(X, Y $$A) ? "yes" : "no");

    string_free(X $$A);
    string_free(Y $$A);

    printf("done.\n");

    return 0;
}
