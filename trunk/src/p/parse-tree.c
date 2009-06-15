/*
 * tree.c
 *
 *  Created on: Jun 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-parse-tree.h>

/**
 * Allocate a new terminal tree.
 */
static PT_Terminal *PT_alloc_terminal(PToken *tok,
                                      uint32_t id) {
    PT_Terminal *tree = NULL;

    assert_not_null(tok);
    assert_not_null(all_trees);

    T = tree_alloc(sizeof(PT_Terminal), 0);
    T->token = tok;
    T->next = NULL;
    T->prev = NULL;
    T->id = id;
    T->_.type = P_PARSE_TREE_TERMINAL;

    return T;
}

/**
 * Allocate a new non-terminal tree.
 */
PT_NonTerminal *PT_alloc_non_terminal(unsigned short num_branches,
                                      G_NonTerminal production) {
    PT_NonTerminal *tree;
    tree = tree_alloc(sizeof(PT_NonTerminal), num_branches);
    tree->_.type = P_PARSE_TREE_PRODUCTION;
    tree->rule = 1;
    tree->production = production;
    return tree;
}

/**
 * Allocate an epsilon tree on the heap.
 */
PT_Epsilon *PT_alloc_epsilon(void) {
    PT_Epsilon *epsilon_tree = tree_alloc(sizeof(PParseTree), 0);
    ((PParseTree *) epsilon_tree)->type = P_PARSE_TREE_EPSILON;
    return epsilon_tree;
}

/**
 * Return a linked list of all tokens in the current file being parsed.
 */
P_TerminalTreeList PT_alloc_terminals(PT_Set *all_trees,
                                      PTokenGenerator *gen) {

    P_TerminalTreeList list;

    PT_Terminal *prev = NULL,
                *curr = NULL;

    uint32_t id = 0;

    assert_not_null(gen);
    assert_not_null(all_trees);

    list.list = NULL;
    list.num_tokens = 0;

    /* build up a list of all of the tokens. */
    if(!generator_next(gen)) {
        return list;
    }

    /* generate the first token */
    list.list = P_tree_alloc_terminal(generator_current(gen), id++);
    prev = list.list;
    parse_tree_set_add(all_trees, prev);

    /* generate the rest of the tokens */
    while(generator_next(gen)) {
        curr = P_tree_alloc_terminal(generator_current(gen), id++);
        parse_tree_set_add(all_trees, curr);
        prev->next = curr;
        curr->prev = prev;
        prev = curr;
    }

    list.num_tokens = id;

    return list;
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
    && branch_tree->type == P_PARSE_TREE_PRODUCTION) {

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


/**
 * Free a parse tree, in its entirety.
 */
void parser_free_parse_tree(PParseTree *parse_tree) {

    assert_not_null(parse_tree);

    tree_free(
        (PTree *) parse_tree,
        (PDelegate) &P_tree_free_token
    );
}

/**
 * Free an intermediate parse tree.
 */
void PT_free_intermediate(PParseTree *parse_tree) {

    PT_Terminal *pt_as_terminal;
    PTree *pt_as_tree;

    assert_not_null(parse_tree);

    pt_as_tree = parse_tree->_;

    /* free up the related token */
    if(parse_tree->type == P_PARSE_TREE_TERMINAL) {
        pt_as_terminal = (PT_Terminal *) parse_tree;

        /* re-link the token chain */
        if(is_not_null(pt_as_terminal->prev)) {
            (pt_as_terminal->prev)->next = pt_as_terminal->next;
        }
        if(is_not_null(pt_as_terminal->next)) {
            (pt_as_terminal->next)->prev = pt_as_terminal->prev;
        }

        if(is_not_null(pt_as_terminal->lexeme)) {
            string_free(pt_as_terminal->lexeme);
        }

        /* unlink this token from the token chain */
        pt_as_terminal->next = NULL;
        pt_as_terminal->prev = NULL;

    } else {
        tree_clear(pt_as_tree, 0);
    }

    tree_free(pt_as_tree, &delegate_do_nothing);
}
