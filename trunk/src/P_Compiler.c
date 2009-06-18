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

#include <p-grammar.h>
#include <p-parser.h>

#define P(x) printf(x)

enum {
    L_NON_TERMINAL,
    L_TERMINAL,
    L_EPSILON,
    L_COLON,
    L_SEMICOLON,
    L_CODE,
    L_DASH,
    L_UP_ARROW
};

enum {
    P_PRODUCTIONS,
    P_PRODUCTION,
    P_PRODUCTION_RULES,
    P_PRODUCTION_RULE,
    P_RULES,
    P_RULE,
    P_DECORATION,
    P_RULE_FLAG,
    P_NON_EXCLUDABLE,
    P_SUBSUMABLE,
    P_TEMP
};

static char production_names[12][16] = {
    "Productions",
    "Production",
    "ProductionRules",
    "ProductionRule",
    "Rules",
    "Rule",
    "Decoration",
    "RuleFlag",
    "NonExludable",
    "Subsumable",

    "Temp"
};

#if 0

typedef int (*char_predicate_t)(int);
typedef struct {
    PString *str;
    char next;
} accumulate_result_t;

/* -------------------------------------------------------------------------- */

static accumulate_result_t accumulate_chars(char_predicate_t predicate,
                                            char *S,
                                            char C,
                                            PTokenGenerator *G) {
    int i;
    accumulate_result_t result;

    for(i = 0; i < 30 && predicate(C); ++i) {
        S[i] = C;
        ++(G->column);

        C = file_get_char(G->stream);
    }
    S[i] = 0;

    result.str = string_alloc_char(S, i);
    result.next = C;

    return result;
}

static int ident_char(int C) {
    return (isalpha(C) || '_' == C);
}

/**
 * Generate a token.
 */
static PToken *lexer_grammar_token_generator(PTokenGenerator *G) {
    char C,
         S[31],
         next_start_char = -1;
    unsigned char token = 0xFF;
    accumulate_result_t result;

    assert_not_null(G);

    result.str = NULL;

    C = G->start_char;

    /* we need to get our first character. */
    if(-1 == C) {
        ++(G->column);
        C = file_get_char(G->stream);

    /* nothing left to lex */
    } else if(EOF == C) {
        return NULL;
    }

    /* skip spaces. */
clear_spaces:
    while(isspace(C)) {
        if(C == '\n') {
            ++(G->line);
            G->column = 0;
        }

        C = file_get_char(G->stream);
        ++(G->column);
    }

    /* line comment */
    if('#' == C) {
        while('\n' != C) {
            if(EOF == C) {
                return NULL;
            }
            C = file_get_char(G->stream);
        }

        ++(G->line);
        G->column = 1;

        C = file_get_char(G->stream);
        goto clear_spaces;
    }

    switch(C) {
        case EOF:
            return NULL;
        case '<':
            C = file_get_char(G->stream);

            if('>' == C) {
                result.str = string_alloc_char("", 0);
                token = L_EPSILON;
                break;
            }

            result = accumulate_chars(&ident_char, S, C, G);

            if(string_length(result.str) == 30 && '>' != result.next) {
                std_error(
                    "Grammar error: non-terminals are limited to 30 characters."
                );
            }

            token = L_TERMINAL;
            break;
        case ':':
            result.str = string_alloc_char(":", 1);
            token = L_COLON;
            break;
        case ';':
            result.str = string_alloc_char(";", 1);
            token = L_SEMICOLON;
            break;
        case '-':
            result.str = string_alloc_char("-", 1);
            token = L_DASH;
            break;
        case '^':
            result.str = string_alloc_char("^", 1);
            token = L_UP_ARROW;
            break;
        default:
            if(ident_char(C)) {
                result = accumulate_chars(&ident_char, S, C, G);
                next_start_char = result.next;
                token = L_NON_TERMINAL;
            } else {
                std_error("Unrecognized character in grammar file.");
                return NULL;
            }
    }

    G->start_char = next_start_char;
    G->lexeme = result.str;
    G->token = token;

    return (void *) 1;
}
#endif

/* -------------------------------------------------------------------------- */

static void print_tree(PParseTree *T) {

    PTreeGenerator *tree_gen = tree_generator_alloc(T, TREE_TRAVERSE_POSTORDER);
    while(generator_next(tree_gen)) {
        T = generator_current(tree_gen);

        if(T->_._parent != NULL) {

            /* terminal */
            if(T->type == 1) {
                printf("\"%p\" -> \"%p\" \n", (void *)T->_._parent, (void *)T);
                if(((PT_Terminal *)T)->lexeme != NULL) {
                    printf("\"%p\" [label=\"%s\" color=blue] \n", (void *)T, (char *)((PT_Terminal *)T)->lexeme->str);
                }
            } else {
                printf("\"%p\" -> \"%p\"\n", (void *)T->_._parent, (void *)T);
            }
        }

        /* production */
        if(T->type == 0) {
            printf("\"%p\" [label=\"%s\"]\n", (void *)T, production_names[((PT_NonTerminal *) T)->production]);
            if(tree_get_num_branches((PTree *) T) == 0) {
                printf("\"%p\" [color=gray]\n", (void *)T);
            }
        /* epsilon */
        } else if (T->type == 2) {

        }
    }
    generator_free(tree_gen);
}

/* -------------------------------------------------------------------------- */

static PGrammar *make_grammar(void) {
    PGrammar *G;

    P("\t allocating grammar. \n");

    G = grammar_alloc(
        P_PRODUCTIONS, /* production to start matching with */
        11, /* number of productions */
        8, /* number of tokens */
        19, /* number of production phrases */
        29 /* number of phrase symbols */
    );

    P("\t grammar allocated. \n");
    P("\t initializing grammar. \n");

            grammar_add_non_terminal_symbol(G, P_TEMP, 0, 1);
            grammar_add_non_terminal_symbol(G, P_PRODUCTION, 0, 0);
        grammar_add_phrase(G);
            grammar_add_epsilon_symbol(G, 0);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_PRODUCTIONS);

            grammar_add_non_terminal_symbol(G, P_PRODUCTIONS, 0, 1);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_TEMP);

            grammar_add_terminal_symbol(G, L_NON_TERMINAL, 1);
            grammar_add_non_terminal_symbol(G, P_PRODUCTION_RULES, 0, 1);
            grammar_add_terminal_symbol(G, L_SEMICOLON, 0);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_PRODUCTION);

            grammar_add_non_terminal_symbol(G, P_PRODUCTION_RULE, 1, 0);
            grammar_add_non_terminal_symbol(G, P_PRODUCTION_RULES, 0, 1);
        grammar_add_phrase(G);
            grammar_add_epsilon_symbol(G, 0);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_PRODUCTION_RULES);

            grammar_add_terminal_symbol(G, L_COLON, 0);
            grammar_add_non_terminal_symbol(G, P_RULES, 0, 1);
            grammar_add_non_terminal_symbol(G, P_DECORATION, 0, 0);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_PRODUCTION_RULE);

            grammar_add_non_terminal_symbol(G, P_RULE, 0, 1),
            grammar_add_non_terminal_symbol(G, P_RULES, 0, 1);
        grammar_add_phrase(G);
            grammar_add_epsilon_symbol(G, 0);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_RULES);

            grammar_add_non_terminal_symbol(G, P_RULE_FLAG, 0, 0);
            grammar_add_terminal_symbol(G, L_NON_TERMINAL, 1);
        grammar_add_phrase(G);
            grammar_add_non_terminal_symbol(G, P_RULE_FLAG, 0, 0);
            grammar_add_terminal_symbol(G, L_TERMINAL, 1);
        grammar_add_phrase(G);
            grammar_add_non_terminal_symbol(G, P_RULE_FLAG, 0, 0);
            grammar_add_terminal_symbol(G, L_EPSILON, 1);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_RULE);

            grammar_add_terminal_symbol(G, L_CODE, 1);
        grammar_add_phrase(G);
            grammar_add_epsilon_symbol(G, 0);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_DECORATION);

            grammar_add_non_terminal_symbol(G, P_NON_EXCLUDABLE, 1, 0);
        grammar_add_phrase(G);
            grammar_add_non_terminal_symbol(G, P_SUBSUMABLE, 1, 0);
        grammar_add_phrase(G);
            grammar_add_epsilon_symbol(G, 0);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_RULE_FLAG);

            grammar_add_terminal_symbol(G, L_DASH, 0);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_NON_EXCLUDABLE);

            grammar_add_terminal_symbol(G, L_UP_ARROW, 0);
        grammar_add_phrase(G);
    grammar_add_production_rule(G, P_SUBSUMABLE);

    P("\t grammar initialized. \n");

    return G;
}

/* -------------------------------------------------------------------------- */

int main(void) {

    int i = 0;
    PGrammar *grammar;
    PToken tokens[20];

    P("Making tokens...\n");

    tokens[i].terminal = L_NON_TERMINAL;
    tokens[i].lexeme = string_alloc_char("Productions", 11);
    tokens[i].line = 2;
    tokens[i++].column = 1;

    tokens[i].terminal = L_COLON;
    tokens[i].lexeme = string_alloc_char(":", 1);
    tokens[i].line = 3;
    tokens[i++].column = 5;

    tokens[i].terminal = L_UP_ARROW;
    tokens[i].lexeme = string_alloc_char("^", 1);
    tokens[i].line = 3;
    tokens[i++].column = 7;

    tokens[i].terminal = L_NON_TERMINAL;
    tokens[i].lexeme = string_alloc_char("Temp", 4);
    tokens[i].line = 3;
    tokens[i++].column = 8;

    tokens[i].terminal = L_NON_TERMINAL;
    tokens[i].lexeme = string_alloc_char("Production", 10);
    tokens[i].line = 3;
    tokens[i++].column = 13;

    tokens[i].terminal = L_COLON;
    tokens[i].lexeme = string_alloc_char(":", 1);
    tokens[i].line = 4;
    tokens[i++].column = 5;

    tokens[i].terminal = L_EPSILON;
    tokens[i].lexeme = string_alloc_char("<>", 2);
    tokens[i].line = 4;
    tokens[i++].column = 7;

    tokens[i].terminal = L_SEMICOLON;
    tokens[i].lexeme = string_alloc_char(":", 1);
    tokens[i].line = 5;
    tokens[i++].column = 5;

    P("\t Tokens made.\n");
    P("Making grammar...\n");

    grammar = make_grammar();

    P("\t Grammar made.\n");

    P("Calling parse_tokens().. \n\n");
    parse_tokens(grammar, tokens, i);
    P("\n\t parse_tokens() returned. \n");

    P("Freeing grammar...\n");
    grammar_free(grammar);
    P("\t Grammar freed.\n");

    P("Freeing token lexemes...\n");
    for(; --i >= 0; ) {
        string_free(tokens[i].lexeme);
    }
    P("\t Token lexemes freed.\n");

#if defined(P_DEBUG) && P_DEBUG == 1 && defined(P_DEBUG_MEM) && P_DEBUG_MEM == 1
    printf("num unfreed pointers: %ld\n", mem_num_allocated_pointers());
#endif

    return 0;
}
