/*
 * grammar.c
 *
 *  Created on: Jul 11, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <pgen-grammar.h>


PGrammar *parser_grammar_grammar(void) {
    PGrammar *G = grammar_alloc(
        P_pg_GrammarRules, /* production to start matching with */
        9, /* number of productions */
        19, /* number of tokens */
        40, /* number of phrases */
        50 /* number of phrase symbols */
    );

    grammar_add_non_terminal_symbol(G, P_pg_Production, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_pg_GrammarRules, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_pg_Terminal, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_pg_GrammarRules, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_GrammarRules);

    grammar_add_terminal_symbol(G, L_pg_non_terminal, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_pg_regexp_1, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_pg_ProductionRules, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_pg_regexp_2, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_Production);

    grammar_add_terminal_symbol(G, L_pg_regexp, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_string, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_subrule_1);

    grammar_add_terminal_symbol(G, L_pg_terminal, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_pg_regexp_1, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_pg_subrule_1, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_pg_regexp_2, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_Terminal);

    grammar_add_non_terminal_symbol(G, P_pg_Rules, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_pg_string_3, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_pg_ProductionRules, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_pg_Rules, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_ProductionRules);

    grammar_add_non_terminal_symbol(G, P_pg_Rule, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_pg_Rules, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_pg_Rule, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_Rules);

    grammar_add_terminal_symbol(G, L_pg_fail, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_cut, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_pg_RuleFlag, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_pg_string_4, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_pg_ProductionRules, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_pg_string_5, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_non_terminal, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_regexp, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_string, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_terminal, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_epsilon, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_subrule_2);

    grammar_add_non_terminal_symbol(G, P_pg_RuleFlag, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_pg_subrule_2, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_Rule);

    grammar_add_terminal_symbol(G, L_pg_non_excludable, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_raise_children, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_kleene_closure, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_positive_closure, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_optional, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_followed_by, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_not_followed_by, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_RuleFlag);

    return G;
}
