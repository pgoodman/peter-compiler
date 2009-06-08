/*
 * tree.c
 *
 *  Created on: Jun 4, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-tree.h>

/**
 * Allocate a new terminal tree.
 */
static PTerminalTree *P_tree_alloc_terminal(P_TreeSet *all_trees, PToken *tok) {
    PTerminalTree *T = NULL;

    assert_not_null(tok);
    assert_not_null(all_trees);

    T = tree_alloc(sizeof(PTerminalTree), 0);
    T->token = tok;
    T->next = NULL;
    T->prev = NULL;
    ((PParseTree *) T)->type = P_PARSE_TREE_TERMINAL;

    dict_set(all_trees, T, T, &delegate_do_nothing);

    return T;
}

/**
 * Allocate a new non-terminal tree.
 */
PProductionTree *P_tree_alloc_non_terminal(unsigned short num_branches,
                                           unsigned char production) {
    PProductionTree *tree;
    tree = tree_alloc(sizeof(PProductionTree), num_branches);
    tree->_.type = P_PARSE_TREE_PRODUCTION;
    tree->rule = 1;
    tree->production = production;
    return tree;
}

/**
 * Allocate an epsilon tree on the heap.
 */
PParseTree *P_tree_alloc_epsilon(void) {
    PParseTree *epsilon_tree = tree_alloc(sizeof(PParseTree), 0);
    ((PParseTree *) epsilon_tree)->type = P_PARSE_TREE_EPSILON;
    return epsilon_tree;
}

/**
 * Return a linked list of all tokens in the current file being parsed.
 */
P_TerminalTreeList P_tree_alloc_terminals(P_TreeSet *all_trees,
                                          PTokenGenerator *gen) {

    P_TerminalTreeList list;
    PTerminalTree *prev = NULL,
                  *curr = NULL;

    assert_not_null(gen);
    assert_not_null(all_trees);

    list.list = NULL;
    list.num_tokens = 0;

    /* build up a list of all of the tokens. */
    if(!generator_next(gen)) {
        return list;
    }

    /* generate the first token */
    list.list = P_tree_alloc_terminal(all_trees, generator_current(gen));
    prev = list.list;
    list.num_tokens = 1;

    /* generate the rest of the tokens */
    while(generator_next(gen)) {
        ++(list.num_tokens);
        curr = P_tree_alloc_terminal(all_trees, generator_current(gen));
        prev->next = curr;
        curr->prev = prev;
        prev = curr;
    }

    return list;
}

/**
 * Free an intermediate parse tree.
 */
static void P_tree_free_intermediate(PParseTree *T) {
    PTerminalTree *term = NULL;
    PTree *node = (PTree *) T;

    assert_not_null(T);

    /* free up the related token */
    if(T->type == P_PARSE_TREE_TERMINAL) {
        term = (PTerminalTree *) T;

        /* re-link the token chain */
        if(is_not_null(term->prev)) {
            (term->prev)->next = term->next;
        }
        if(is_not_null(term->next)) {
            (term->next)->prev = term->prev;
        }

        token_free(term->token);

        /* unlink this token from the token chain */
        term->next = NULL;
        term->prev = NULL;
        term->token = NULL;

    } else {
        tree_clear(node, 0);
    }

    tree_free(node, &delegate_do_nothing);
}

/**
 * Free the tokens from a parse tree node.
 */
static void P_tree_free_token(PParseTree *tree) {
    PTerminalTree *term;

    assert_not_null(tree);

    if(tree->type == P_PARSE_TREE_TERMINAL) {
        term = (PTerminalTree *) tree;
        if(is_not_null(term->token)) {
            token_free(term->token);
        }
    }
}

/**
 * Record the parse tree for the successful application of a parse tree.
 */
void P_tree_record_production(PParseTree *temp_tree,
                              PParseTree *parse_tree,
                              PParserRewriteRule *curr_rule) {

    unsigned short j = tree_get_num_branches((PTree *) temp_tree);

    if(!P_adt_rule_is_non_excludable(curr_rule)
    && j <= 1
    && temp_tree->type == P_PARSE_TREE_PRODUCTION) {

        /* this is a production with only one child filled,
         * promote that single child node to the place of this
         * production in the tree and ignore that production. */
        if(j > 0) {
            tree_force_add_branch(
                (PTree *) parse_tree,
                tree_get_branch(
                    (PTree *) temp_tree,
                    0
                )
            );
        }

    /* this node must be added to the tree, has more than one child, or must
     * have its children included in the parse tree. */
    } else {
        if(P_adt_rule_use_children_instead(curr_rule)) {
            tree_force_add_branch_children(
                (PTree *) parse_tree,
                (PTree *) temp_tree
            );
        } else {
            tree_force_add_branch(
                (PTree *) parse_tree,
                (PTree *) temp_tree
            );
        }
    }

    return;
}

/**
 * Allocate a set of trees.
 */
P_TreeSet *P_tree_set_alloc(void) {
    return dict_alloc(
        53,
        &dict_pointer_hash_fnc,
        &dict_pointer_collision_fnc
    );
}

/**
 * Free a set of trees.
 */
void P_tree_set_free(P_TreeSet *set) {
    assert_not_null(set);
    dict_free(
        set,
        &delegate_do_nothing,
        (PDelegate) &P_tree_free_intermediate
    );
}

/**
 * Add a parse tree to the tree set.
 */
void P_tree_set_add(P_TreeSet *set, PParseTree *tree) {
    assert_not_null(set);
    assert_not_null(tree);
    dict_set(set, tree, tree, &delegate_do_nothing);
}

/**
 * Rmove a tree from the tree set.
 */
void P_tree_set_remove(P_TreeSet *set, PParseTree *tree) {
    assert_not_null(set);
    assert_not_null(tree);

    dict_unset(
        set,
        tree,
        &delegate_do_nothing,
        &delegate_do_nothing
    );
}

/**
 * Check if progress was made in parsing.
 */
int P_tree_progress_was_made(PTerminalTree *tree,
                             unsigned int line,
                             unsigned int column) {
    PToken *tok;
    if(is_null(tree)) {
        return 0;
    } else {
        tok = tree->token;
        return (tok->line > line)
            || (tok->line == line && tok->column > column);
    }
}

/**
 * Free a parse tree, in its entirety.
 */
void parser_free_parse_tree(PParseTree *tree) {
    assert_not_null(tree);
    tree_free(
        (PTree *) tree,
        (PDelegate) &P_tree_free_token
    );
}
