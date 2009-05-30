/*
 * lexer.c
 *
 *  Created on: May 13, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-lexer.h>

static unsigned long int num_allocations = 0;

#define token_mem_alloc(x) mem_alloc(x); ++num_allocations
#define token_mem_calloc(x,y) mem_calloc(x,y); ++num_allocations
#define token_mem_free(x) mem_free(x); --num_allocations
#define token_mem_error(x) mem_error(x)

unsigned long int token_num_allocated_pointers(void) {
    return num_allocations;
}

/**
 * Allocate a new token.
 */
PToken *token_alloc(char lexeme, PString *val, uint32_t line, uint32_t col) {

    PToken *tok = token_mem_alloc(sizeof(PToken *));
    if(is_null(tok)) {
        token_mem_error("Unable to allocate a token on the heap.");
    }

    tok->lexeme = lexeme;
    tok->val = val;
    tok->line = line;
    tok->column = col;

    return tok;
}

/**
 * Free a token.
 */
void token_free(PToken *tok) {
    assert_not_null(tok);

    if(is_not_null(tok->val)) {
        string_free(tok->val);
        tok->val = NULL;
    }

    token_mem_free(tok);
}

/**
 * Free a token generator.
 */
static void L_generator_free(PTokenGenerator *G) {
    assert_not_null(G);
    assert_not_null(G->stream);

    file_free(G->stream);
    G->stream = NULL;

    token_mem_free(G);
}

/**
 * Allocate a new token generator. The next function is the function that
 * actually lexes input. This allows the token generator to be completely
 * general.
 */
PTokenGenerator *token_generator_alloc(const char *file_name,
                                       PFunction gen_next_fnc) {
    PTokenGenerator *G = NULL;
    PFileInputStream *S = NULL;

    assert_not_null(file_name);
    assert_not_null(generator_next);

    S = file_read_char(file_name);
    G = generator_alloc(sizeof(PTokenGenerator));

    generator_init(
        G,
        gen_next_fnc,
        (PDelegate) &L_generator_free
    );

    G->stream = S;
    G->column = 1;
    G->line = 1;
    G->start_char = -1;

    return G;
}
