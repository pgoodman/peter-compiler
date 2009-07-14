
#include <c-grammar.h>

PGrammar *lang_grammar(void) {
    PGrammar *G = grammar_alloc(
        P_lang_Program, /* production to start matching with */
        16, /* number of productions */
        10, /* number of tokens */
        28, /* number of phrases */
        57 /* number of phrase symbols */
    );

    grammar_add_non_terminal_symbol(G, P_lang_StatementList, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_Program);

    grammar_add_terminal_symbol(G, L_lang_string_1, G_AUTO);
    grammar_add_terminal_symbol(G, L_lang_string_2, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_lang_TypeList, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_lang_string_3, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_lang_Type, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_lang_string_2, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_FunctionType);

    grammar_add_terminal_symbol(G, L_lang_type_name, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_lang_string_4, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_lang_FunctionType, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_lang_string_5, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_Type);

    grammar_add_non_terminal_symbol(G, P_lang_Type, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_lang_TypeList, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_TypeList);

    grammar_add_terminal_symbol(G, L_lang_identifier, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_lang_IdentifierList, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_IdentifierList);

    grammar_add_non_terminal_symbol(G, P_lang_Type, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_lang_identifier, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_TypedIdentifier);

    grammar_add_non_terminal_symbol(G, P_lang_TypedIdentifier, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_lang_IdentifierList, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_lang_TypedIdentifier, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_TypeDestructure);

    grammar_add_non_terminal_symbol(G, P_lang_TypedIdentifier, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_lang_string_4, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_lang_TypeDestructure, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_lang_string_5, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_TypedParameter);

    grammar_add_non_terminal_symbol(G, P_lang_TypedParameter, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_lang_TypedParameterList, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_TypedParameterList);

    grammar_add_terminal_symbol(G, L_lang_string_2, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_lang_TypedParameterList, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_lang_string_3, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_lang_Type, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_lang_string_2, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_FunctionHeader);

    grammar_add_terminal_symbol(G, L_lang_integer, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_lang_string, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_lang_identifier, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_Atom);

    grammar_add_terminal_symbol(G, L_lang_string_4, G_AUTO);
    grammar_add_terminal_symbol(G, L_lang_identifier, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_lang_ExpressionList, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_lang_string_5, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_FunctionApplication);

    grammar_add_non_terminal_symbol(G, P_lang_Atom, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_lang_FunctionApplication, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_Expression);

    grammar_add_non_terminal_symbol(G, P_lang_Expression, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_lang_ExpressionList, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_ExpressionList);

    grammar_add_non_terminal_symbol(G, P_lang_Expression, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_lang_StatementList, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_lang_FunctionDefinition, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_lang_StatementList, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_StatementList);

    grammar_add_terminal_symbol(G, L_lang_string_4, G_AUTO);
    grammar_add_terminal_symbol(G, L_lang_string_6, G_AUTO);
    grammar_add_terminal_symbol(G, L_lang_identifier, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_lang_FunctionHeader, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_lang_StatementList, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_lang_string_5, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_lang_FunctionDefinition);

    return G;
}
