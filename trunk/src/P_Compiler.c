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
#include <p-lexer.h>
#include <p-adt.h>
#include <adt-dict.h>

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
    P_SUBSUMABLE
};

typedef int (*char_predicate_t)(int);
typedef struct {
    PString *str;
    char next;
} accumulate_result_t;

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
         lexeme = -1,
         next_start_char = -1;
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
                lexeme = L_EPSILON;
                break;
            }

            result = accumulate_chars(&ident_char, S, C, G);

            if(string_length(result.str) == 30 && '>' != result.next) {
                std_error(
                    "Grammar error: non-terminals are limited to 30 characters."
                );
            }

            lexeme = L_TERMINAL;
            break;
        case ':':
            result.str = string_alloc_char(":", 1);
            lexeme = L_COLON;
            break;
        case ';':
            result.str = string_alloc_char(";", 1);
            lexeme = L_SEMICOLON;
            break;
        case '-':
            result.str = string_alloc_char("-", 1);
            lexeme = L_DASH;
            break;
        case '^':
            result.str = string_alloc_char("^", 1);
            lexeme = L_UP_ARROW;
            break;
        default:
            if(ident_char(C)) {
                result = accumulate_chars(&ident_char, S, C, G);
                next_start_char = result.next;
                lexeme = L_NON_TERMINAL;
            } else {
                std_error("Unrecognized character in grammar file.");
                return NULL;
            }
    }

    G->start_char = next_start_char;
    return token_alloc(lexeme, result.str, G->line, G->column);
}

static char production_names[10][16] = {
    "Productions",
    "Production",
    "ProductionRules",
    "ProductionRule",
    "Rules",
    "Rule",
    "Decoration",
    "RuleFlag",
    "NonExludable",
    "Subsumable"
};

static void print_tree(PParseTree *T) {
    PTreeGenerator *tree_gen = tree_generator_alloc(T, TREE_TRAVERSE_POSTORDER);
    while(generator_next(tree_gen)) {
        T = generator_current(tree_gen);

        if(T->_._parent != NULL) {

            /* terminal */
            if(T->type == 1) {
                printf("\"%p\" -> \"%p\" \n", (void *)T->_._parent, (void *)T);
                if(((PTerminalTree *)T)->token->val != NULL) {
                    printf("\"%p\" [label=\"%s\" color=blue] \n", (void *)T, (char *)((PTerminalTree *)T)->token->val->str);
                }
            } else {
                printf("\"%p\" -> \"%p\"\n", (void *)T->_._parent, (void *)T);
            }
        }

        /* production */
        if(T->type == 0) {
            printf("\"%p\" [label=\"%s\"]\n", (void *)T, production_names[((PProductionTree *) T)->production]);
            if(tree_get_num_branches((PTree *) T) == 0) {
                printf("\"%p\" [color=gray]\n", (void *)T);
            }
        /* epsilon */
        } else if (T->type == 2) {

        }
    }

    generator_free(tree_gen);
}

int main(void) {

    PParser *P;
    PParseTree *T;
    PTokenGenerator *G = token_generator_alloc(
        "src/grammars/parser.g",
        (PFunction) &lexer_grammar_token_generator
    );

    P = parser_alloc(
        P_PRODUCTIONS, /* production to start matching with */
        10, /* number of productions */
        8 /* number of tokens */
    );

    parser_add_production(P, P_PRODUCTIONS, 2,
        parser_rule_sequence(P, 2,
            parser_rewrite_production(P, P_PRODUCTION, 0),
            parser_rewrite_production(P, P_PRODUCTIONS, 2)),
        parser_rule_sequence(P, 1,
            parser_rewrite_epsilon(P, 0)));

    parser_add_production(P, P_PRODUCTION, 1,
        parser_rule_sequence(P, 3,
            parser_rewrite_token(P, L_NON_TERMINAL, 1),
            parser_rewrite_production(P, P_PRODUCTION_RULES, 2),
            parser_rewrite_token(P, L_SEMICOLON, 0)));

    parser_add_production(P, P_PRODUCTION_RULES, 2,
        parser_rule_sequence(P, 2,
            parser_rewrite_production(P, P_PRODUCTION_RULE, 1),
            parser_rewrite_production(P, P_PRODUCTION_RULES, 2)),
        parser_rule_sequence(P, 1,
            parser_rewrite_epsilon(P, 0)));

    parser_add_production(P, P_PRODUCTION_RULE, 1,
        parser_rule_sequence(P, 3,
            parser_rewrite_token(P, L_COLON, 0),
            parser_rewrite_production(P, P_RULES, 2),
            parser_rewrite_production(P, P_DECORATION, 0)));

    parser_add_production(P, P_RULES, 2,
        parser_rule_sequence(P, 2,
            parser_rewrite_production(P, P_RULE, 2),
            parser_rewrite_production(P, P_RULES, 2)),
        parser_rule_sequence(P, 1,
            parser_rewrite_epsilon(P, 0)));

    parser_add_production(P, P_RULE, 3,
        parser_rule_sequence(P, 2,
            parser_rewrite_production(P, P_RULE_FLAG, 0),
            parser_rewrite_token(P, L_NON_TERMINAL, 1)),
        parser_rule_sequence(P, 2,
            parser_rewrite_production(P, P_RULE_FLAG, 0),
            parser_rewrite_token(P, L_TERMINAL, 1)),
        parser_rule_sequence(P, 2,
            parser_rewrite_production(P, P_RULE_FLAG, 0),
            parser_rewrite_token(P, L_EPSILON, 1)));

    parser_add_production(P, P_DECORATION, 2,
        parser_rule_sequence(P, 1,
            parser_rewrite_token(P, L_CODE, 1)),
        parser_rule_sequence(P, 1,
            parser_rewrite_epsilon(P, 0)));

    parser_add_production(P, P_RULE_FLAG, 3,
        parser_rule_sequence(P, 1,
            parser_rewrite_production(P, P_NON_EXCLUDABLE, 1)),
        parser_rule_sequence(P, 1,
            parser_rewrite_production(P, P_SUBSUMABLE, 1)),
        parser_rule_sequence(P, 1,
            parser_rewrite_epsilon(P, 0)));

    parser_add_production(P, P_NON_EXCLUDABLE, 1,
        parser_rule_sequence(P, 1,
            parser_rewrite_token(P, L_DASH, 0)));

    parser_add_production(P, P_SUBSUMABLE, 1,
        parser_rule_sequence(P, 1,
            parser_rewrite_token(P, L_UP_ARROW, 0)));

    /* parse the grammar file */
    T = parser_parse_tokens(P, G);


    printf("root %p \n\n", (void *)T);

    print_tree(T);


    generator_free(G);
    parser_free(P);
    parser_free_parse_tree(T);


#if defined(P_DEBUG) && P_DEBUG == 1 && defined(P_DEBUG_MEM) && P_DEBUG_MEM == 1
    printf("num unfreed pointers: %ld\n", mem_num_allocated_pointers());
#endif

    printf(
        "dict: %ld \ntree: %ld\ntokens: %ld \nstrings: %ld \n",
        dict_num_allocated_pointers(),
        tree_num_allocated_pointers(),
        token_num_allocated_pointers(),
        string_num_allocated_pointers()
    );

    printf("done.\n");

    return 0;
}
