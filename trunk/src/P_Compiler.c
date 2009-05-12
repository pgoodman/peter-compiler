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
#include <adt-dict.h>
#include <func-delegate.h>
#include <p-parser.h>
#include <func-delegate.h>

typedef struct CharPTree {
    PTree _;
    char letter;
} CharTree;

unsigned int __st_depth = 0;

int main() { $MH

    PParser *P = parser_alloc($A);

#if 0
    size_t s = sizeof(CharTree);
    PTreeGenerator *gen = NULL;
    PStack *S = NULL;

    printf("before alloc trees.\n");

    CharTree *A = tree_alloc($$A s, 3 ),
             *B = tree_alloc($$A s, 3 ),
             *C = tree_alloc($$A s, 0 ),
             *D = tree_alloc($$A s, 2 ),
             *E = tree_alloc($$A s, 0 ),
             *F = tree_alloc($$A s, 1 ),
             *G = tree_alloc($$A s, 0 ),
             *H = tree_alloc($$A s, 1 ),
             *I = tree_alloc($$A s, 0 ),
             *J = tree_alloc($$A s, 0 ),
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

    tree_add_branch($$A A, B );
        tree_add_branch($$A B, E );
        tree_add_branch($$A B, F );
            tree_add_branch($$A F, H );
        tree_add_branch($$A B, G );
    tree_add_branch($$A A, C );
    tree_add_branch($$A A, D );
        tree_add_branch($$A D, I );
        tree_add_branch($$A D, J );

    /* try out the post-ordering generator */
    printf("Post-order:\n");
    gen = tree_generator_alloc($$A A, TREE_TRAVERSE_POSTORDER );
    while(generator_next($$A gen)) {
        curr = generator_current($$A gen );
        printf("\t%c\n", curr->letter);
    }
    generator_free($$A gen );

    /* try out the pre-ordering generator */
    printf("Pre-order (depth-first):\n");
    gen = tree_generator_alloc($$A A, TREE_TRAVERSE_PREORDER );
    while(generator_next($$A gen)) {
        curr = generator_current($$A gen );
        printf("\t%c\n", curr->letter);
    }
    generator_free($$A gen );

    /* try out the level ordering generator */
    printf("Level-order (breadth-first):\n");
    gen = tree_generator_alloc($$A A, TREE_TRAVERSE_LEVELORDER );
    while(generator_next($$A gen)) {
        curr = generator_current($$A gen );
        printf("\t%c\n", curr->letter);
    }
    generator_free($$A gen );

    /* try out tree trimming the branches of and internal node */
    S = stack_alloc($$A sizeof(PStack) );
    tree_trim($$A B, S );

    /* see what the post-order traversal looks like */
    printf("Post-order AFTER tree trimming the branches of B:\n");
    gen = tree_generator_alloc($$A A, TREE_TRAVERSE_POSTORDER );
    while(generator_next($$A gen)) {
        curr = generator_current($$A gen );
        printf("\t%c\n", curr->letter);
    }
    generator_free($$A gen );

    printf("freeing resources..\n");

    stack_free($$A S, &PDelegateree_free );
    tree_free($$A A, &delegate_do_nothing );

    printf("resources freed, testing vector operations.\n");

    PVector *V = vector_alloc($$A 10 );
    vector_set($$A V, 0, tree_alloc($$A s, 4), delegate_do_nothing );
    vector_free($$A V, PDelegateree_free );

    printf("vector operations work.\n");
    printf("trying hash table operations.\n");

    PDictionary *table = dict_alloc($$A 100, &dict_hash_pointer );
    int a = 10;

    printf("adding/getting 100 hash table elements.\n");

    for(; a < 100; ++a) {
        dict_set($$A table, (void *) a, (void *) a, &delegate_do_nothing );
        dict_get($$A table, (void *) a );
    }

    printf("removing all 100 elements.\n");

    for(a = 10; a < 100; ++a) {
        dict_unset($$A table, (void *) a, &delegate_do_nothing );
    }

    printf("freeeing table.\n");

    dict_free($$A table, &delegate_do_nothing );

    printf("table freed.\n");

    printf("testing string stuff.\n");

    PString *X = string_alloc_char($$A "hello world", 11 ),
            *Y = string_alloc_char($$A "hello world", 11 );

    printf("are these strings equal? %s\n", string_equal($$A X, Y ) ? "yes" : "no");

    string_free($$A X );
    string_free($$A Y );

    printf("done.\n");
#endif
    return 0;
}
