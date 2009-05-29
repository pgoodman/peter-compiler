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
    L_CODE
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
            lexeme = L_COLON;
            break;
        case ';':
            lexeme = L_SEMICOLON;
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

static void Grammar(PProductionTree *T, PDictionary *garbage) {

}

static void Productions(PProductionTree *T, PDictionary *garbage) {
    if(T->rule == 1) {
    } else {

    }
}

static void Production(PProductionTree *T, PDictionary *garbage) {
}

static void ProductionRules(PProductionTree *T, PDictionary *garbage) {
}

static void ProductionRule(PProductionTree *T, PDictionary *garbage) {
}

static void Rules(PProductionTree *T, PDictionary *garbage) {
}

static void Rule(PProductionTree *T, PDictionary *garbage) {
}

static void Decoration(PProductionTree *T, PDictionary *garbage) {
}

#if defined(P_DEBUG) && P_DEBUG == 1
#if defined(P_DEBUG_MEM) && P_DEBUG_MEM == 1
extern unsigned int num_allocated_pointers;
#endif
#endif

int main(void) {

    PParser *P;
    PParseTree *T;
    PTokenGenerator *G = token_generator_alloc(
        "src/grammars/parser.g",
        (PFunction) &lexer_grammar_token_generator
    );

    PTreeGenerator *tree_gen;

    short useful_tokens[] = {
        L_TERMINAL,
        L_NON_TERMINAL,
        L_CODE,
        L_EPSILON
    };

    P = parser_alloc(&Grammar, 6, 4, useful_tokens);

    parser_add_production(P, &Grammar, 1,
        parser_rule_sequence(P, 2,
            parser_rewrite_function(P, &Production),
            parser_rewrite_function(P, &Productions)));

    parser_add_production(P, &Productions, 2,
        parser_rule_sequence(P, 1,
            parser_rewrite_function(P, &Grammar)),
        parser_rule_sequence(P, 1,
            parser_rewrite_epsilon(P)));

    parser_add_production(P, &Production, 1,
        parser_rule_sequence(P, 3,
            parser_rewrite_token(P, L_NON_TERMINAL),
            parser_rewrite_function(P, &ProductionRules),
            parser_rewrite_token(P, L_SEMICOLON)));

    parser_add_production(P, &ProductionRules, 2,
        parser_rule_sequence(P, 2,
            parser_rewrite_function(P, &ProductionRule),
            parser_rewrite_function(P, &ProductionRules)),
        parser_rule_sequence(P, 1,
            parser_rewrite_epsilon(P)));

    parser_add_production(P, &ProductionRule, 1,
        parser_rule_sequence(P, 2,
            parser_rewrite_token(P, L_COLON),
            parser_rewrite_function(P, &Rules)));

    parser_add_production(P, &Rules, 2,
        parser_rule_sequence(P, 2,
            parser_rewrite_function(P, &Rule),
            parser_rewrite_function(P, &Rules)),
        parser_rule_sequence(P, 1,
            parser_rewrite_epsilon(P)));

    parser_add_production(P, &Rule, 3,
        parser_rule_sequence(P, 1,
            parser_rewrite_token(P, L_NON_TERMINAL)),
        parser_rule_sequence(P, 1,
            parser_rewrite_token(P, L_TERMINAL)),
        parser_rule_sequence(P, 1,
            parser_rewrite_token(P, L_EPSILON)));

    parser_add_production(P, &Decoration, 2,
        parser_rule_sequence(P, 1,
            parser_rewrite_token(P, L_CODE)),
        parser_rule_sequence(P, 1,
            parser_rewrite_epsilon(P)));

    /* parse the grammar file */
    T = parser_parse_tokens(P, G);

    generator_free(G);
    parser_free(P);
    parser_free_parse_tree(T);

    /*
    tree_gen = tree_generator_alloc(T, TREE_TRAVERSE_PREORDER);
    while(generator_next(tree_gen)) {
        T = (PParseTree *)generator_current(tree_gen);
        printf("%p fill %d degree %d \n", (void *)T, (T->_)._fill, (T->_)._degree);
    }
    generator_free(tree_gen);*/

#if defined(P_DEBUG) && P_DEBUG == 1
#if defined(P_DEBUG_MEM) && P_DEBUG_MEM == 1
    printf("num unfreed pointers: %d\n", num_allocated_pointers);
#endif
#endif

    printf("done.\n");

    return 0;
}
