/*
 ============================================================================
 Name        : P_Compiler.c
 Author      : Peter Goodman
 Version     :
 ============================================================================
 */

#include <string.h>
#include <ctype.h>

#include <std-include.h>
#include <std-string.h>

#include <adt-nfa.h>

#include <p-scanner.h>
#include <p-grammar.h>
#include <p-regexp.h>

#include <math.h>

#include "gen/parser.g.h"

#define P fprintf

enum {
    L_NON_TERMINAL,
    L_TERMINAL,
    L_EPSILON,
    L_REGEXP,
    L_COLON,
    L_SEMICOLON,
    L_DASH,
    L_UP_ARROW,
    L_CUT,
    L_FAIL
};

enum {
    P_GRAMMAR_RULES,
    P_PRODUCTION,
    P_TERMINAL,
    P_PRODUCTION_RULES,
    P_PRODUCTION_RULE,
    P_RULES,
    P_RULE,
    P_RULE_FLAG,
    P_NON_EXCLUDABLE,
    P_RAISE_CHILDREN
};

typedef struct {
    PDictionary *terminals,
                *non_terminals,
                *production_rules,
                *regexps;
    PScanner *scanner;
    FILE *fp;
    char *lexer_output_file;
    unsigned int num_symbols,
                 num_phrases;
    PString *first_production;
} PParserInfo;

/* print out the branches and terminals in the DOT graph language */
static void print_branches(unsigned int num_branches, PParseTree *branches[]) {
    unsigned int i;
    PTree *t;
    for(i = 0; i < num_branches; ++i) {
        t = (PTree *) branches[i];
        if(is_not_null(t->_branches)) {
            printf(
                "Ox%d -> Ox%d\n",
                (unsigned int) branches,
                (unsigned int) t->_branches
            );
        } else if(branches[i]->type == PT_TERMINAL) {
            printf(
                "Ox%d -> Ox%d\n",
                (unsigned int) branches,
                (unsigned int) t
            );

            printf(
                "Ox%d [label=\"%s\" color=blue]\n",
                (unsigned int) t,
                ((PT_Terminal *) t)->lexeme->str
            );
        }
    }
}

/* -------------------------------------------------------------------------- */

static void I_Production(PParserInfo *state,
                        unsigned char phrase,
                        unsigned int num_branches,
                        PParseTree *branches[]) {
    PString *production_name = ((PT_Terminal *) branches[0])->lexeme;
    if(dict_is_set(state->production_rules, production_name)) {
        std_error("Grammar Error: A Production rule cannot be defined twice.");
    }
    dict_set(state->production_rules, production_name, NULL, &delegate_do_nothing);
    state->num_phrases += num_branches - 1;

    if(is_null(state->first_production)) {
        state->first_production = production_name;
    }
}

static void I_Terminal(PParserInfo *state,
                     unsigned char phrase,
                     unsigned int num_branches,
                     PParseTree *branches[]) {
    PString *str = ((PT_Terminal *) branches[1])->lexeme;
    unsigned int j;

    /* get rid of leading and trailing ' */
    for(j = 1; j < str->len; ++j) {
        str->str[j-1] = str->str[j];
    }
    str->str[j-1] = 0;

    dict_set(
        state->terminals,
        ((PT_Terminal *) branches[0])->lexeme, /* name */
        str, /* regexp */
        &delegate_do_nothing
    );
}

static void I_ProductionRule(PParserInfo *state,
                     unsigned char phrase,
                     unsigned int num_branches,
                     PParseTree *branches[]) {
    static unsigned int k = 0;
    unsigned int i = 0, j;
    PT_Terminal *term;
    PString *str;
    char term_name[15],
         *c;

    for(; i < num_branches; ++i) {
        if(branches[i]->type == PT_NON_TERMINAL) {
            continue;
        }

        term = (PT_Terminal *) branches[i];

        switch(term->terminal) {
            case L_NON_TERMINAL:
                ++(state->num_symbols);
                dict_set(
                    state->non_terminals,
                    term->lexeme,
                    NULL,
                    &delegate_do_nothing
                );
                break;
            case L_TERMINAL:
                ++(state->num_symbols);
                if(!dict_is_set(state->terminals, term->lexeme)) {
                    std_error("Grammar Error: Undefined terminal symbol.");
                }
                break;
            case L_REGEXP:
                ++(state->num_symbols);

                /* get rid of the leading and trailing ' */
                for(j = 1; j < term->lexeme->len; ++j) {
                    term->lexeme->str[j-1] = term->lexeme->str[j];
                }
                term->lexeme->str[j-1] = 0;

                if(!dict_is_set(state->regexps, term->lexeme)) {
                    sprintf(term_name, "regexp_%d", ++k);
                    for(c = term_name, j = 0; *c; ++j, ++c)
                        ;
                    str = string_alloc_char(term_name, j);

                    dict_set(
                        state->regexps,
                        term->lexeme,
                        str,
                        &delegate_do_nothing
                    );
                    dict_set(
                        state->terminals,
                        str,
                        term->lexeme,
                        &delegate_do_nothing
                    );
                }
                break;
            case L_CUT:
            case L_EPSILON:
                ++(state->num_symbols);
                break;
            default:
                break;
        }
    }
}

/* -------------------------------------------------------------------------- */

static void R_make_scanner(PParserInfo *state) {
    PDictionaryGenerator *keys = dict_keys_generator_alloc(state->terminals),
                         *values = dict_values_generator_alloc(state->terminals);
    PString *regexp,
            *key;
    PGrammar *grammar = regexp_grammar();
    PNFA *nfa = nfa_alloc(),
         *dfa;
    unsigned int start,
                 i = 0;
    char *sep = "    ";

    start = nfa_add_state(nfa);
    nfa_change_start_state(nfa, start);

    P(state->fp, "\n\n");
    P(state->fp, "#include <p-scanner.h>\n");
    P(state->fp, "#include <p-grammar.h>\n");
    P(state->fp, "#include <p-parser.h>\n\n");
    P(state->fp, "enum {\n");

    for(; generator_next(keys) && generator_next(values); ++i) {
        key = generator_current(keys);
        regexp = generator_current(values);

        regexp_parse(
            grammar,
            state->scanner,
            nfa,
            (unsigned char *) regexp->str,
            start,
            i
        );

        P(state->fp, "%sL_%s=%d", sep, key->str, i);
        sep = ",\n    ";
    }

    P(state->fp, "\n};\n\n");

    grammar_free(grammar);

    dfa = nfa_to_dfa(nfa);
    nfa_free(nfa);

    nfa_print_scanner(dfa, state->lexer_output_file, "tokenize");

    nfa_free(dfa);
    generator_free(keys);
    generator_free(values);

    sep = "    ";
    keys = dict_keys_generator_alloc(state->non_terminals);
    P(state->fp, "enum {\n");

    for(i = 0; generator_next(keys); ++i) {
        key = generator_current(keys);

        if(!dict_is_set(state->production_rules, key)) {
            std_error(
                "Grammar Error: Grammar contains non-terminal that does not "
                "have an associated production rule."
            );
        }

        P(state->fp, "%sP_%s=%d", sep, key->str, i);
        sep = ",\n    ";
    }

    P(state->fp, "\n};\n\n");

    generator_free(keys);

    P(state->fp, "extern PGrammar *make_grammar(void);");
    P(state->fp, "PGrammar *make_grammar(void) {\n");
    P(state->fp, "    PGrammar *G = grammar_alloc(\n");
    P(state->fp, "        P_%s, /* production to start matching with */\n", state->first_production->str);
    P(state->fp, "        %d, /* number of productions */\n", dict_size(state->production_rules));
    P(state->fp, "        %d, /* number of tokens */\n", dict_size(state->terminals));
    P(state->fp, "        %d, /* number of phrases */\n", state->num_phrases);
    P(state->fp, "        %d /* number of phrase symbols */\n", state->num_symbols);
    P(state->fp, "    );\n\n");
}

/* -------------------------------------------------------------------------- */

static void C_Production(PParserInfo *state,
                        unsigned char phrase,
                        unsigned int num_branches,
                        PParseTree *branches[]) {
    char *production_name = ((PT_Terminal *) branches[0])->lexeme->str;
    P(state->fp, "    grammar_add_production_rule(G, P_%s);\n\n", production_name);
}

static void C_ProductionRule(PParserInfo *state,
                     unsigned char phrase,
                     unsigned int num_branches,
                     PParseTree *branches[]) {
    static unsigned int k = 0;
    unsigned int i = 0, j;
    PT_Terminal *term;
    PT_NonTerminal *nterm;
    PString *str;

    char term_name[15],
         *c,
         *modifier = "G_AUTO";

    for(; i < num_branches; ++i) {
        if(branches[i]->type == PT_NON_TERMINAL) {
            nterm = (PT_NonTerminal *) branches[i];
            if(nterm->production == P_NON_EXCLUDABLE) {
                modifier = "G_NON_EXCLUDABLE";
            } else {
                modifier = "G_RAISE_CHILDREN";
            }
            continue;
        }

        term = (PT_Terminal *) branches[i];

        switch(term->terminal) {
            case L_NON_TERMINAL:
                P(state->fp, "    grammar_add_non_terminal_symbol(G, P_%s, %s);\n", term->lexeme->str, modifier);
                break;
            case L_TERMINAL:
                P(state->fp, "    grammar_add_terminal_symbol(G, L_%s, %s);\n", term->lexeme->str, modifier);
                break;
            case L_REGEXP:
                str = dict_get(state->regexps, term->lexeme);
                P(state->fp, "    grammar_add_terminal_symbol(G, L_%s, %s);\n", str->str, modifier);
                break;
            case L_CUT:
                P(state->fp, "    grammar_add_cut_symbol(G);\n");
                break;
            case L_FAIL:
                P(state->fp, "    grammar_add_fail_symbol(G);\n");
                break;
            case L_EPSILON:
                P(state->fp, "    grammar_add_epsilon_symbol(G, %s);\n", modifier);
                break;
            default:
                break;
        }

        modifier = "G_AUTO";
    }

    P(state->fp, "    grammar_add_phrase(G);\n");
}

/* -------------------------------------------------------------------------- */

static void C_finish_parser(PParserInfo *state) {
    P(state->fp, "    return G;\n");
    P(state->fp, "}\n\n");
}

/* -------------------------------------------------------------------------- */

static PGrammar *parser_grammar_grammar(void) {

    PGrammar *G = grammar_alloc(
        P_GRAMMAR_RULES, /* production to start matching with */
        10,              /* number of productions */
        9,               /* number of tokens */
        20,              /* number of production phrases */
        34               /* number of phrase symbols */
    );

    G_ProductionRuleFunc *symbol_table_actions[] = {
        &grammar_null_action,    /* P_GRAMMAR_RULES */
        (G_ProductionRuleFunc *) &I_Production,      /* P_PRODUCTION */
        (G_ProductionRuleFunc *) &I_Terminal,        /* P_TERMINAL */
        &grammar_null_action, /* P_PRODUCTION_RULES */
        (G_ProductionRuleFunc *) &I_ProductionRule,  /* P_PRODUCTION_RULE */
        &grammar_null_action,   /* P_RULES */
        &grammar_null_action,   /* P_RULE */
        &grammar_null_action,   /* P_RULE_FLAG */
        &grammar_null_action,   /* P_NON_EXCLUDABLE */
        &grammar_null_action    /* P_RAISE_CHILDREN */
    };

    G_ProductionRuleFunc *code_gen_actions[] = {
        &grammar_null_action,    /* P_GRAMMAR_RULES */
        (G_ProductionRuleFunc *) &C_Production,      /* P_PRODUCTION */
        &grammar_null_action,        /* P_TERMINAL */
        &grammar_null_action, /* P_PRODUCTION_RULES */
        (G_ProductionRuleFunc *) &C_ProductionRule,  /* P_PRODUCTION_RULE */
        &grammar_null_action,   /* P_RULES */
        &grammar_null_action,   /* P_RULE */
        &grammar_null_action,   /* P_RULE_FLAG */
        &grammar_null_action,   /* P_NON_EXCLUDABLE */
        &grammar_null_action    /* P_RAISE_CHILDREN */
    };

    grammar_add_tree_actions(G, TREE_TRAVERSE_POSTORDER, symbol_table_actions);
    grammar_add_state_action(G, (PDelegate *) &R_make_scanner);
    grammar_add_tree_actions(G, TREE_TRAVERSE_POSTORDER, code_gen_actions);
    grammar_add_state_action(G, (PDelegate *) &C_finish_parser);

    grammar_add_non_terminal_symbol(G, P_GRAMMAR_RULES, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_PRODUCTION, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_GRAMMAR_RULES, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_TERMINAL, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_GRAMMAR_RULES);

    grammar_add_terminal_symbol(G, L_NON_TERMINAL, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_PRODUCTION_RULES, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_SEMICOLON, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_PRODUCTION);

    grammar_add_terminal_symbol(G, L_TERMINAL, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_COLON, G_AUTO);
    grammar_add_terminal_symbol(G, L_REGEXP, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_SEMICOLON, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_TERMINAL);

    grammar_add_non_terminal_symbol(G, P_PRODUCTION_RULE, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_PRODUCTION_RULES, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_PRODUCTION_RULES);

    grammar_add_terminal_symbol(G, L_COLON, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_RULES, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_PRODUCTION_RULE);

    grammar_add_non_terminal_symbol(G, P_RULE, G_RAISE_CHILDREN),
    grammar_add_non_terminal_symbol(G, P_RULES, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_RULES);

    grammar_add_non_terminal_symbol(G, P_RULE_FLAG, G_AUTO);
    grammar_add_terminal_symbol(G, L_NON_TERMINAL, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_RULE_FLAG, G_AUTO);
    grammar_add_terminal_symbol(G, L_REGEXP, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_RULE_FLAG, G_AUTO);
    grammar_add_terminal_symbol(G, L_TERMINAL, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_RULE_FLAG, G_AUTO);
    grammar_add_terminal_symbol(G, L_EPSILON, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_CUT, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_RULE);

    grammar_add_non_terminal_symbol(G, P_NON_EXCLUDABLE, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_RAISE_CHILDREN, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_RULE_FLAG);

    grammar_add_terminal_symbol(G, L_DASH, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_NON_EXCLUDABLE);

    grammar_add_terminal_symbol(G, L_UP_ARROW, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_RAISE_CHILDREN);

    return G;
}

/* -------------------------------------------------------------------------- */

int main(void) {
    PScanner *scanner = scanner_alloc();
    PGrammar *grammar = parser_grammar_grammar();
    PParserInfo info;

    char *grammar_input_file = "src/grammars/parser.g",
         *grammar_output_file = "src/gen/grammar.h",
         *lexer_output_file = "src/gen/lexer.h";

    info.non_terminals = dict_alloc(
        53,
        (PDictionaryHashFunc *) &string_hash_fnc,
        (PDictionaryCollisionFunc *) &string_collision_fnc
    );
    info.regexps = dict_alloc(
        53,
        (PDictionaryHashFunc *) &string_hash_fnc,
        (PDictionaryCollisionFunc *) &string_collision_fnc
    );
    info.terminals = dict_alloc(
        53,
        (PDictionaryHashFunc *) &string_hash_fnc,
        (PDictionaryCollisionFunc *) &string_collision_fnc
    );
    info.production_rules = dict_alloc(
        53,
        (PDictionaryHashFunc *) &string_hash_fnc,
        (PDictionaryCollisionFunc *) &string_collision_fnc
    );

    info.scanner = scanner;
    info.lexer_output_file = lexer_output_file;
    info.fp = fopen(grammar_output_file, "w");
    info.num_symbols = 0;
    info.num_phrases = 0;
    info.first_production = NULL;

    if(is_null(info.fp)) {
        std_error("Internal Parser Generator Error: Unable to create output file.");
    }

    if(scanner_use_file(scanner, grammar_input_file)) {
        scanner_flush(scanner, 1);
        parse_tokens(
            grammar,
            scanner,
            (PScannerFunc *) &parser_grammar_lexer,
            &info
        );
    }

    dict_free(info.production_rules, &delegate_do_nothing, &delegate_do_nothing);
    dict_free(info.non_terminals, &delegate_do_nothing, &delegate_do_nothing);
    dict_free(info.terminals, &delegate_do_nothing, &delegate_do_nothing);
    dict_free(
        info.regexps,
        &delegate_do_nothing,
        (PDictionaryFreeValueFunc *) &string_free
    );

    scanner_free(scanner);
    grammar_free(grammar);

    fclose(info.fp);

#if 0
    PScanner *scanner = scanner_alloc();
    PGrammar *grammar = regexp_grammar();
    PNFA *nfa = nfa_alloc();
    unsigned int start = nfa_add_state(nfa);

    nfa_change_start_state(nfa, start);
    regexp_parse(
        grammar,
        scanner,
        nfa,
        (unsigned char *) "ab",
        start,
        L_UP_ARROW
    );

    nfa_free(nfa);
    grammar_free(grammar);
    scanner_free(scanner);
#endif

#if defined(P_DEBUG) && P_DEBUG == 1 && defined(P_DEBUG_MEM) && P_DEBUG_MEM == 1
    printf("num unfreed pointers: %ld\n", mem_num_allocated_pointers());
#endif

    return 0;
}
