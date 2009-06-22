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
static PT_Terminal *PT_alloc_terminal(G_Terminal terminal,
                                      PString *lexeme,
                                      uint32_t line,
                                      uint32_t column,
                                      uint32_t id) {
    PT_Terminal *tree = NULL;

    tree = tree_alloc(sizeof(PT_Terminal), 0);
    ((PParseTree *) tree)->type = PT_TERMINAL;

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
    tree->rule = 1;
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
 * Return a linked list of all tokens in the current file being parsed.
 */
PT_Terminal *PT_alloc_terminals(PT_Set *all_trees,
                                PToken tokens[],
                                int num_tokens) {

    PT_Terminal *curr = NULL,
                *next;

    unsigned int id;

    assert_not_null(all_trees);

    /* make the EOF token tree */
    next = PT_alloc_terminal(0, NULL, 0, 0, num_tokens);
    next->next = NULL;
    curr = next;

    PTS_add(all_trees, (PParseTree *) curr);

    /* make the main set of tokens */
    if(num_tokens > 0) {
        for(id = num_tokens; id > 0; ) {
            --id;

            curr = PT_alloc_terminal(
                tokens[id].terminal,
                tokens[id].lexeme,
                tokens[id].line,
                tokens[id].column,
                id
            );

            PTS_add(all_trees, (PParseTree *) curr);

            curr->next = next;
            next = curr;
        }
    }

    return curr;
}

/**
 * Record the parse tree for the successful application of a parse tree.
 */
void PT_add_branch(PParseTree *parse_tree,
                   PParseTree *branch_tree,
                   G_Symbol *symbol) {

    unsigned short num_branches = tree_get_num_branches((PTree *) branch_tree);

    if(!G_symbol_is_non_excludable(symbol)
    && num_branches <= 1
    && branch_tree->type == PT_NON_TERMINAL) {

        /* this is a production with only one child filled,
         * promote that single child node to the place of this
         * production in the tree and ignore that production. */
        if(num_branches > 0) {
            tree_force_add_branch(
                (PTree *) parse_tree,
                tree_get_branch(
                    (PTree *) branch_tree,
                    0
                )
            );
        }

    /* this node must be added to the tree, has more than one child, or must
     * have its children included in the parse tree. */
    } else {
        if(G_symbol_use_children_instead(symbol)) {
            tree_force_add_branch_children(
                (PTree *) parse_tree,
                (PTree *) branch_tree
            );
        } else {
            tree_force_add_branch(
                (PTree *) parse_tree,
                (PTree *) branch_tree
            );
        }
    }

    return;
}

static void PT_clear(PParseTree *parse_tree) {
    PT_Terminal *pt_as_terminal;
    if(parse_tree->type == PT_TERMINAL) {
        pt_as_terminal = (PT_Terminal *) parse_tree;
        if(is_not_null(pt_as_terminal->lexeme)) {
            string_free(pt_as_terminal->lexeme);
            pt_as_terminal->lexeme = NULL;
        }

    } else if(parse_tree->type == PT_NON_TERMINAL) {
        tree_clear((PTree *) parse_tree, 0);
    }
}

/**
 * Free a parse tree (excluding its branches).
 */
static void PT_free(PParseTree *parse_tree) {
    PT_clear(parse_tree);
    tree_free((PTree *) parse_tree, &delegate_do_nothing);
}

/* -------------------------------------------------------------------------- */

/**
 * Free a parse tree, in its entirety.
 */
void parser_free_parse_tree(PParseTree *parse_tree) {

    assert_not_null(parse_tree);

    tree_free(
        (PTree *) parse_tree,
        (PDelegate) &PT_clear
    );
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a set of trees.
 */
PT_Set *PTS_alloc(void) {
    return dict_alloc(
        53,
        &dict_pointer_hash_fnc,
        &dict_pointer_collision_fnc
    );
}

/**
 * Free a set of trees.
 */
void PTS_free(PT_Set *set) {
    assert_not_null(set);
    dict_free(
        set,
        &delegate_do_nothing, /* free key */
        (PDelegate) &PT_free /* free val */
    );
}

/**
 * Add a parse tree to the tree set.
 */
void PTS_add(PT_Set *set, PParseTree *tree) {
    assert_not_null(set);
    assert_not_null(tree);
    dict_set(set, tree, tree, &delegate_do_nothing);
}

/**
 * Remove a tree from the tree set.
 */
void PTS_remove(PT_Set *set, PParseTree *tree) {
    assert_not_null(set);
    assert_not_null(tree);
    dict_unset(
        set,
        tree,
        &delegate_do_nothing,
        &delegate_do_nothing
    );
}
