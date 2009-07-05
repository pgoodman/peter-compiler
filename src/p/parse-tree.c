/*
 * tree.c
 *
 *  Created on: Jun 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-parse-tree.h>
#include <p-grammar-internal.h>

/**
 * Allocate a new terminal tree.
 */
PT_Terminal *PT_alloc_terminal(G_Terminal terminal,
                               PString *lexeme,
                               uint32_t line,
                               uint32_t column,
                               uint32_t id) {
    PT_Terminal *tree = NULL;

    tree = tree_alloc(sizeof(PT_Terminal), 0);
    ((PParseTree *) tree)->type = PT_TERMINAL;

    /*
    if(is_not_null(lexeme)) {
        printf("lexeme: { %s } %d \n", lexeme->str, terminal);
    }
    */

    tree->terminal = terminal;
    tree->lexeme = lexeme;
    tree->line = line;
    tree->column = column;
    tree->next = NULL;
    tree->id = id;

    return tree;
}

/**
 * Allocate a new non-terminal tree.
 */
PT_NonTerminal *PT_alloc_non_terminal(G_NonTerminal production,
                                      unsigned short num_branches) {
    PT_NonTerminal *tree;
    tree = tree_alloc(sizeof(PT_NonTerminal), num_branches);
    ((PParseTree *) tree)->type = PT_NON_TERMINAL;
    tree->phrase = 0;
    tree->production = production;
    return tree;
}

/**
 * Allocate an epsilon tree on the heap.
 */
PT_Epsilon *PT_alloc_epsilon(void) {
    PT_Epsilon *tree = tree_alloc(sizeof(PT_Epsilon), 0);
    ((PParseTree *) tree)->type = PT_EPSILON;
    return tree;
}

/**
 * Clear the branches or the lexeme of a parse tree.
 */
static void PT_clear(PParseTree *parse_tree) {
    PT_Terminal *pt_as_terminal;
    if(parse_tree->type == PT_TERMINAL) {

        pt_as_terminal = (PT_Terminal *) parse_tree;

        if(is_not_null(pt_as_terminal->lexeme)) {
            string_free(pt_as_terminal->lexeme);
            pt_as_terminal->lexeme = NULL;
        }

    } else if(parse_tree->type == PT_NON_TERMINAL) {
        tree_clear((PTree *) parse_tree);
    }
}

/**
 * Free a parse tree (excluding its branches).
 */
static void PT_free(PParseTree *parse_tree) {
    PT_Terminal *term;
    PT_clear(parse_tree);
    tree_free((PTree *) parse_tree, &delegate_do_nothing);
}

/* -------------------------------------------------------------------------- */

/**
 * Free a parse tree, in its entirety.
 */
void parse_tree_free(PParseTree *parse_tree) {
    tree_free(
        (PTree *) parse_tree,
        (PDelegate *) &PT_clear
    );
}

/**
 * Print the parse tree out in the DOT language.
 */
void parse_tree_print_dot(PParseTree *parse_tree,
                          char production_names[][40],
                          char terminal_names[][40]) {
    PTreeGenerator *gen;
    PParseTree *curr;
    PTree *tree;
    PT_Terminal *term;
    unsigned int i;

    if(is_null(parse_tree)) {
        return;
    }

    gen = tree_generator_alloc(parse_tree, TREE_TRAVERSE_LEVELORDER);
    while(generator_next(gen)) {

        curr = (PParseTree *) generator_current(gen);
        tree = (PTree *) curr;

        switch(curr->type) {
            case PT_NON_TERMINAL:
                printf(
                   "Ox%d [label=\"%s\"] \n",
                   (unsigned int) tree,
                   production_names[((PT_NonTerminal *) curr)->production]
                );

                for(i = 0; i < tree->_fill; ++i) {
                    printf(
                        "Ox%d -> Ox%d \n",
                        (unsigned int) tree,
                        (unsigned int) tree->_branches[i]
                    );
                }

                break;

            case PT_TERMINAL:
                term = (PT_Terminal *) curr;
                printf(
                    "Ox%d [label=\"%s<%s> @ %d\" color=gray shape=square] \n",
                    (unsigned int) tree,
                    terminal_names[term->terminal],
                    is_not_null(term->lexeme) ? term->lexeme->str : "",
                    term->id
                );

                break;

            case PT_EPSILON:
                printf(
                    "Ox%d [label=\"epsilon<>\" color=gray shape=diamond] \n",
                    (unsigned int) tree
                );
                break;
        }
    }

    generator_free(gen);
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a set of trees.
 */
PT_Set *PTS_alloc(void) {
    return dict_alloc(
        53,
        (PDictionaryHashFunc *) &dict_pointer_hash_fnc,
        (PDictionaryCollisionFunc *) &dict_pointer_collision_fnc
    );
}

/**
 * Free a set of trees.
 */
void PTS_free(PT_Set *set) {
    dict_free(
        set,
        &delegate_do_nothing, /* free key */
        (PDelegate *) &PT_free /* free val */
    );
}

/**
 * Add a parse tree to the tree set.
 */
void PTS_add(PT_Set *set, PParseTree *tree) {
    dict_set(set, tree, tree, &delegate_do_nothing);
}

/**
 * Remove a tree from the tree set.
 */
void PTS_remove(PT_Set *set, PParseTree *tree) {
    dict_unset(
        set,
        tree,
        &delegate_do_nothing,
        &delegate_do_nothing
    );
}

/**
 * Return the number of elements in the tree set.
 */
uint32_t PTS_size(PT_Set *set) {
    return dict_size(set);
}
