

#include <p-scanner.h>
#include <p-grammar.h>
#include <p-parser.h>

enum {
    L_epsilon=0,
    L_regexp=1,
    L_regexp_4=2,
    L_regexp_3=3,
    L_regexp_2=4,
    L_non_terminal=5,
    L_regexp_1=6,
    L_terminal=7,
    L_cut=8,
    L_fail=9
};

enum {
    P_RaiseChildren=0,
    P_RuleFlag=1,
    P_ProductionRules=2,
    P_Terminal=3,
    P_Production=4,
    P_Rule=5,
    P_Rules=6,
    P_NonExcludable=7,
    P_GrammarRules=8,
    P_ProductionRule=9
};

extern PGrammar *make_grammar(void);

PGrammar *make_grammar(void) {
    PGrammar *G = grammar_alloc(
        P_GrammarRules, /* production to start matching with */
        10, /* number of productions */
        10, /* number of tokens */
        21, /* number of phrases */
        35 /* number of phrase symbols */
    );

    grammar_add_non_terminal_symbol(G, P_GrammarRules, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_Production, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_GrammarRules, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_Terminal, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_GrammarRules);

    grammar_add_terminal_symbol(G, L_non_terminal, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_ProductionRules, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_regexp_1, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_Production);

    grammar_add_terminal_symbol(G, L_terminal, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_regexp_2, G_AUTO);
    grammar_add_terminal_symbol(G, L_regexp, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_regexp_1, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_Terminal);

    grammar_add_non_terminal_symbol(G, P_ProductionRule, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_ProductionRules, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_ProductionRules);

    grammar_add_terminal_symbol(G, L_regexp_2, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_Rules, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_ProductionRule);

    grammar_add_non_terminal_symbol(G, P_Rule, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_Rules, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_Rules);

    grammar_add_terminal_symbol(G, L_fail, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_RuleFlag, G_AUTO);
    grammar_add_terminal_symbol(G, L_non_terminal, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_RuleFlag, G_AUTO);
    grammar_add_terminal_symbol(G, L_regexp, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_RuleFlag, G_AUTO);
    grammar_add_terminal_symbol(G, L_terminal, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_RuleFlag, G_AUTO);
    grammar_add_terminal_symbol(G, L_epsilon, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_cut, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_Rule);

    grammar_add_non_terminal_symbol(G, P_NonExcludable, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_RaiseChildren, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_RuleFlag);

    grammar_add_terminal_symbol(G, L_regexp_3, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_NonExcludable);

    grammar_add_terminal_symbol(G, L_regexp_4, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_RaiseChildren);

    return G;
}

