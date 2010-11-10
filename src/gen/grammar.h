

#ifndef _PGEN_parser_grammar_grammar_
#define _PGEN_parser_grammar_grammar_
#include <p-scanner.h>
#include <p-grammar.h>
#include <p-parser.h>

enum {
    L_pg_string_5=0,
    L_pg_epsilon=1,
    L_pg_string_4=2,
    L_pg_string_3=3,
    L_pg_string_2=4,
    L_pg_string_1=5,
    L_pg_regexp=6,
    L_pg_non_terminal=7,
    L_pg_non_excludable=8,
    L_pg_terminal=9,
    L_pg_self=10,
    L_pg_cut=11,
    L_pg_raise_children=12,
    L_pg_string=13,
    L_pg_fail=14
};

enum {
    P_pg_RuleFlag=0,
    P_pg_ProductionRules=1,
    P_pg_Terminal=2,
    P_pg_Production=3,
    P_pg_Rule=4,
    P_pg_GrammarRules=5,
    P_pg_Rules=6,
    P_pg_subrule_1=7,
    P_pg_subrule_2=8
};

extern PGrammar *parser_grammar_grammar(void);

PGrammar *parser_grammar_grammar(void) {
    PGrammar *G = grammar_alloc(
        P_pg_GrammarRules, /* production to start matching with */
        9, /* number of productions */
        15, /* number of tokens */
        24, /* number of phrases */
        39 /* number of phrase symbols */
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
    grammar_add_terminal_symbol(G, L_pg_string_1, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_pg_ProductionRules, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_pg_string_2, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_Production);

    grammar_add_terminal_symbol(G, L_pg_regexp, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_pg_string, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_subrule_1);

    grammar_add_terminal_symbol(G, L_pg_terminal, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_pg_string_1, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_pg_subrule_1, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_pg_string_2, G_RAISE_CHILDREN);
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
    grammar_add_terminal_symbol(G, L_pg_self, G_NON_EXCLUDABLE);
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
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_pg_RuleFlag);

    return G;
}
#endif

