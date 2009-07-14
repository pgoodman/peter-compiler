/*
 * gen.c
 *
 *  Created on: Jul 11, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

/*
 ============================================================================
 Name        : P_Compiler.c
 Author      : Peter Goodman
 Version     :
 ============================================================================
 */

#include <pgen-gen.h>

#define P fprintf
#define D(x) x

typedef struct {
    PDictionary *terminals,
                *non_terminals,
                *production_rules,
                *regexps,
                *strings,
                *sub_rules;
    PScanner *scanner;
    FILE *fp;

    char *lexer_output_file,
         *lexer_func_name,
         *grammar_func_name,
         *language_name;

    unsigned int num_symbols,
                 num_phrases;
    PString *first_production;
} PParserInfo;

/* -------------------------------------------------------------------------- */

static void I_Production(PParserInfo *state,
                        unsigned char phrase,
                        unsigned int num_branches,
                        PParseTree *branches[]) {
    PString *production_name = ((PT_Terminal *) branches[0])->lexeme;

    if(dict_is_set(state->production_rules, production_name)) {
        D( printf("Production rule: %s \n", production_name->str); )
        std_error("Grammar Error: A Production rule cannot be defined twice.");
    }

    D( printf("Found production '%s' \n", production_name->str); )

    dict_set(state->production_rules, production_name, NULL, &delegate_do_nothing);
    state->num_phrases += num_branches - 1;

    if(is_null(state->first_production)) {
        state->first_production = production_name;
    }
}

/**
 * Remove a single leading and trailing single-quote from the regular
 * expression.
 */
static PString *trim_regexp(PString *regexp) {

    unsigned int j;
    for(j = 1; j < regexp->len; ++j) {
        regexp->str[j-1] = regexp->str[j];
    }
    regexp->str[regexp->len - 2] = 0;
    regexp->len -= 2;

    return regexp;
}

static void record_regexp(PDictionary *regexps_dict,
                          PDictionary *terminals_dict,
                          PString *regexp,
                          PString *term_name) {
    dict_set(
        regexps_dict,
        regexp,
        term_name,
        &delegate_do_nothing
    );
    dict_set(
        terminals_dict,
        term_name,
        regexp,
        &delegate_do_nothing
    );
}

static void I_Terminal(PParserInfo *state,
                     unsigned char phrase,
                     unsigned int num_branches,
                     PParseTree *branches[]) {

    PT_Terminal *term_regexp = (PT_Terminal *) branches[1];
    PString *regexp = trim_regexp(term_regexp->lexeme),
            *terminal = ((PT_Terminal *) branches[0])->lexeme;

    D( printf("Found terminal '%s' -> {%s} \n", terminal->str, regexp->str); )

    record_regexp(
        (term_regexp->terminal == L_pg_regexp ? state->regexps : state->strings),
        state->terminals,
        regexp,
        string_copy(terminal)
    );
}

static void I_Rules(PParserInfo *state,
                     unsigned char phrase,
                     unsigned int num_branches,
                     PParseTree *branches[]) {
    static unsigned int k = 0, l = 0;
    unsigned int i = 0, j;
    PT_Terminal *term;
    PString *str;
    char term_name[15],
         *c;

    for(; i < num_branches; ++i) {
        if(branches[i]->type == PT_NON_TERMINAL) {

            ++(state->num_symbols);
            state->num_phrases += num_branches - 1;

            sprintf(term_name, "subrule_%d", ++l);
            for(c = term_name, j = 0; *c; ++j, ++c)
                ;
            str = string_alloc_char(term_name, j);
            dict_set(
                state->sub_rules,
                tree_get_branches(branches[i]),
                str,
                &delegate_do_nothing
            );
            continue;
        }

        term = (PT_Terminal *) branches[i];

        switch(term->terminal) {
            case L_pg_non_terminal:
                ++(state->num_symbols);
                dict_set(
                    state->non_terminals,
                    term->lexeme,
                    NULL,
                    &delegate_do_nothing
                );
                break;
            case L_pg_terminal:
                ++(state->num_symbols);
                if(!dict_is_set(state->terminals, term->lexeme)) {
                    D( printf("Terminal symbol: %s \n", term->lexeme->str); )
                    std_error("Grammar Error: Undefined terminal symbol.");
                }
                break;
            case L_pg_regexp:
                ++(state->num_symbols);

                trim_regexp(term->lexeme);

                if(!dict_is_set(state->regexps, term->lexeme)) {
                    sprintf(term_name, "regexp_%d", ++k);
                    for(c = term_name, j = 0; *c; ++j, ++c)
                        ;
                    str = string_alloc_char(term_name, j);

                    record_regexp(
                        state->regexps,
                        state->terminals,
                        term->lexeme,
                        str
                    );

                    D( printf(
                        "Found in-line regular expression '%s' -> {%s} \n",
                        term_name,
                        term->lexeme->str
                    ); )
                }
                break;
            case L_pg_string:
                ++(state->num_symbols);

                trim_regexp(term->lexeme);

                if(!dict_is_set(state->strings, term->lexeme)) {
                    sprintf(term_name, "string_%d", ++k);
                    for(c = term_name, j = 0; *c; ++j, ++c)
                        ;
                    str = string_alloc_char(term_name, j);

                    record_regexp(
                        state->strings,
                        state->terminals,
                        term->lexeme,
                        str
                    );

                    D( printf(
                        "Found in-line string '%s' -> {%s} \n",
                        term_name,
                        term->lexeme->str
                    ); )
                }
                break;
            case L_pg_cut:
            case L_pg_fail:
            case L_pg_epsilon:
                ++(state->num_symbols);
                break;
            default:
                break;
        }
    }
}

/* -------------------------------------------------------------------------- */

/**
 * Create the scanner/lexer/tokenizer and also begin the creation of the grammar
 * file.
 */
static void R_make_scanner(PParserInfo *state) {

    PDictionaryGenerator *keys,
                         *values;

    PString *regexp,
            *key;

    PGrammar *grammar = regexp_grammar();

    PNFA *nfa = nfa_alloc(),
         *dfa;

    PSet *priority_set = set_alloc();

    unsigned int start,
                 i;

    char *sep = "    ";

    D( printf("checking non-terminals against production rules... \n"); )

    /* perform some error checking by making sure that every non-terminal has
     * and associated production rule. */
    keys = dict_keys_generator_alloc(state->non_terminals);
    for(i = 0; generator_next(keys); ) {
        key = generator_current(keys);
        if(!dict_is_set(state->production_rules, key)) {
            D( printf("Non-terminal symbol: %s \n", key->str); )
            std_error(
                "Grammar Error: Grammar contains non-terminal that does not "
                "have an associated production rule."
            );
        }
    }
    generator_free(keys);

    start = nfa_add_state(nfa);
    nfa_change_start_state(nfa, start);

    P(state->fp, "\n\n");
    P(state->fp, "#ifndef _PGEN_%s_\n", state->grammar_func_name);
    P(state->fp, "#define _PGEN_%s_\n", state->grammar_func_name);
    P(state->fp, "#include <p-scanner.h>\n");
    P(state->fp, "#include <p-grammar.h>\n");
    P(state->fp, "#include <p-parser.h>\n\n");
    P(state->fp, "enum {\n");

    D( printf("creating lexer... \n"); )

    keys = dict_keys_generator_alloc(state->terminals);
    values = dict_values_generator_alloc(state->terminals);

    /* print out all of the terminal names and parse the various regular
     * expressions. Parsing is done here because the order that we are printing
     * them out is associated with the decision id for each non-terminal. */
    for(i = 0; generator_next(keys) && generator_next(values); ++i) {
        key = generator_current(keys);
        regexp = generator_current(values);

        if(dict_is_set(state->strings, regexp)) {
            D( printf("parsing string expression {%s}...\n", regexp->str); )
            set_add_elm(priority_set, regexp_parse_cat(
                grammar,
                state->scanner,
                nfa,
                (unsigned char *) regexp->str,
                start,
                i /* decision / conclusion */
            ));
        } else {
            D( printf("parsing regular expression {%s}...\n", regexp->str); )
            regexp_parse(
                grammar,
                state->scanner,
                nfa,
                (unsigned char *) regexp->str,
                start,
                i /* decision / conclusion */
            );
        }
        D( printf("done parsing.\n"); )

        P(state->fp, "%sL_%s_%s=%d", sep, state->language_name, key->str, i);
        sep = ",\n    ";
    }

    P(state->fp, "\n};\n\n");

    generator_free(keys);
    generator_free(values);
    grammar_free(grammar);

    /* convert the now constructed NFA of all of the regular expressions that
     * match lexemes and associate then with terminals into a DFA. Once that has
     * been done, print the DFA out as a scanner (in C code) to a file. */
    dfa = nfa_to_dfa(nfa, priority_set);
    set_free(priority_set);
    nfa_free(nfa);
    nfa_print_scanner(dfa, state->lexer_output_file, state->lexer_func_name);
    nfa_free(dfa);

    D( printf("creating head of grammar file... \n"); )

    /* now go and list out the production rules in an enum so that they can be
     * referenced by the grammar data structure by a meaningful name. */
    sep = "    ";
    keys = dict_keys_generator_alloc(state->production_rules);
    P(state->fp, "enum {\n");

    for(i = 0; generator_next(keys); ++i) {
        key = generator_current(keys);
        P(state->fp, "%sP_%s_%s=%d", sep, state->language_name, key->str, i);
        sep = ",\n    ";
    }
    generator_free(keys);

    /* go and list out the production sub-rules */
    keys = dict_values_generator_alloc(state->sub_rules);
    for(; generator_next(keys); ++i) {
        key = generator_current(keys);
        P(state->fp, "%sP_%s_%s=%d", sep, state->language_name, key->str, i);
        sep = ",\n    ";
    }
    generator_free(keys);

    P(state->fp, "\n};\n\n");

    /* print out the header of the grammar builing function. */
    P(state->fp, "extern PGrammar *%s(void);\n\n", state->grammar_func_name);
    P(state->fp, "PGrammar *%s(void) {\n", state->grammar_func_name);
    P(state->fp, "    PGrammar *G = grammar_alloc(\n");
    P(state->fp, "        P_%s_%s, /* production to start matching with */\n", state->language_name, state->first_production->str);
    P(state->fp, "        %d, /* number of productions */\n", i);
    P(state->fp, "        %d, /* number of tokens */\n", dict_size(state->terminals));
    P(state->fp, "        %d, /* number of phrases */\n", state->num_phrases);
    P(state->fp, "        %d /* number of phrase symbols */\n", state->num_symbols);
    P(state->fp, "    );\n\n");

    /* the next step is postorder traversal of the parse tree that prints out
     * the various calls to grammar building functions. */
}

/* -------------------------------------------------------------------------- */

/* deal with normal production rules */
static void C_Production(PParserInfo *state,
                        unsigned char phrase,
                        unsigned int num_branches,
                        PParseTree *branches[]) {
    char *production_name = ((PT_Terminal *) branches[0])->lexeme->str;
    P(
        state->fp,
        "    grammar_add_production_rule(G, P_%s_%s);\n\n",
        state->language_name,
        production_name
    );
}

/* Deal with anonymous production rules. */
static void C_ProductionRules(PParserInfo *state,
                        unsigned char phrase,
                        unsigned int num_branches,
                        PParseTree *branches[]) {
    PString *production_name;
    if(!dict_is_set(state->sub_rules, branches)) {
        return;
    }
    production_name = dict_get(state->sub_rules, branches);
    P(
        state->fp,
        "    grammar_add_production_rule(G, P_%s_%s);\n\n",
        state->language_name,
        production_name->str
    );
}

/* deal with the symbols of a phrase */
static void C_Rules(PParserInfo *state,
                     unsigned char phrase,
                     unsigned int num_branches,
                     PParseTree *branches[]) {
    unsigned int i = 0;
    PT_Terminal *term;
    PT_NonTerminal *nterm;
    PString *str;

    char *modifier = "G_AUTO";

    for(; i < num_branches; ++i) {

        if(branches[i]->type == PT_NON_TERMINAL) {
            str = dict_get(state->sub_rules, tree_get_branches(branches[i]));
            P(
                state->fp,
                "    grammar_add_non_terminal_symbol(G, P_%s_%s, %s);\n",
                state->language_name,
                str->str,
                modifier
            );
            continue;
        }

        term = (PT_Terminal *) branches[i];

        switch(term->terminal) {
            case L_pg_non_excludable:
                modifier = "G_NON_EXCLUDABLE";
                goto next_iteration;
            case L_pg_raise_children:
                modifier = "G_RAISE_CHILDREN";
                goto next_iteration;
            case L_pg_non_terminal:
                P(
                    state->fp,
                    "    grammar_add_non_terminal_symbol(G, P_%s_%s, %s);\n",
                    state->language_name,
                    term->lexeme->str,
                    modifier
                );
                break;
            case L_pg_terminal:
                P(
                    state->fp,
                    "    grammar_add_terminal_symbol(G, L_%s_%s, %s);\n",
                    state->language_name,
                    term->lexeme->str,
                    modifier
                );
                break;
            case L_pg_regexp:
                str = dict_get(state->regexps, term->lexeme);
                P(
                    state->fp,
                    "    grammar_add_terminal_symbol(G, L_%s_%s, %s);\n",
                    state->language_name,
                    str->str,
                    modifier
                );
                break;
            case L_pg_string:
                str = dict_get(state->strings, term->lexeme);
                P(
                    state->fp,
                    "    grammar_add_terminal_symbol(G, L_%s_%s, %s);\n",
                    state->language_name,
                    str->str,
                    modifier
                );
                break;
            case L_pg_cut:
                P(state->fp, "    grammar_add_cut_symbol(G);\n");
                break;
            case L_pg_fail:
                P(state->fp, "    grammar_add_fail_symbol(G);\n");
                break;
            case L_pg_epsilon:
                P(
                    state->fp,
                    "    grammar_add_epsilon_symbol(G, %s);\n",
                    modifier
                );
                break;
            default:
                break;
        }

        modifier = "G_AUTO";

        next_iteration:
        str = NULL; /* dummy */
    }

    P(state->fp, "    grammar_add_phrase(G);\n");
}

/* -------------------------------------------------------------------------- */

/**
 * Finish off the grammar building function and grammar file.
 */
static void C_finish_parser(PParserInfo *state) {
    P(state->fp, "    return G;\n");
    P(state->fp, "}\n");
    P(state->fp, "#endif\n\n");
}

/* -------------------------------------------------------------------------- */

void parser_gen(char *grammar_input_file,
                char *grammar_func_name,
                char *grammar_output_file,
                char *lexer_func_name,
                char *lexer_output_file,
                char *language_name) {

    PScanner *scanner = scanner_alloc();
    PGrammar *grammar = parser_grammar_grammar();
    PParserInfo info;

    G_ProductionRuleFunc *symbol_table_actions[] = {
        &grammar_null_action,
        &grammar_null_action,
        (G_ProductionRuleFunc *) &I_Terminal,
        (G_ProductionRuleFunc *) &I_Production,
        &grammar_null_action,
        &grammar_null_action,
        (G_ProductionRuleFunc *) &I_Rules
    };

    G_ProductionRuleFunc *code_gen_actions[] = {
        &grammar_null_action,
        (G_ProductionRuleFunc *) &C_ProductionRules,
        &grammar_null_action,
        (G_ProductionRuleFunc *) &C_Production,
        &grammar_null_action,
        &grammar_null_action,
        (G_ProductionRuleFunc *) &C_Rules
    };

    grammar_add_tree_actions(grammar, TREE_TRAVERSE_POSTORDER, symbol_table_actions);
    grammar_add_state_action(grammar, (PDelegate *) &R_make_scanner);
    grammar_add_tree_actions(grammar, TREE_TRAVERSE_POSTORDER, code_gen_actions);
    grammar_add_state_action(grammar, (PDelegate *) &C_finish_parser);

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
    info.strings = dict_alloc(
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
    info.sub_rules = dict_alloc(
        53,
        &dict_pointer_hash_fnc,
        &dict_pointer_collision_fnc
    );

    info.scanner = scanner;
    info.lexer_output_file = lexer_output_file;
    info.lexer_func_name = lexer_func_name;
    info.grammar_func_name = grammar_func_name;
    info.fp = fopen(grammar_output_file, "w");
    info.num_symbols = 0;
    info.num_phrases = 0;
    info.first_production = NULL;
    info.language_name = language_name;

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
        info.sub_rules,
        &delegate_do_nothing,
        (PDictionaryFreeValueFunc *) &string_free
    );
    dict_free(
        info.regexps,
        &delegate_do_nothing,
        (PDictionaryFreeValueFunc *) &string_free
    );
    dict_free(
        info.strings,
        &delegate_do_nothing,
        (PDictionaryFreeValueFunc *) &string_free
    );

    scanner_free(scanner);
    grammar_free(grammar);

    fclose(info.fp);
}
