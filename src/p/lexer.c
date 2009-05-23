/*
 * lexer.c
 *
 *  Created on: May 13, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-lexer.h>

/**
 * Allocate a new token.
 */
PToken *token_alloc(char lexeme, PString *val, uint32_t line, uint32_t col) {

    PToken *tok = mem_alloc(sizeof(PToken *));
    if(is_null(tok)) {
        mem_error("Unable to allocate a token on the heap.");
    }

    tok->lexeme = lexeme;
    tok->val = val;
    tok->line = line;
    tok->column = col;

    return tok;
}

/**
 * Free a token generator.
 */
static void L_generator_free(PTokenGenerator *G) {
    assert_not_null(G);
    assert_not_null(G->stream);

    file_free(G->stream);
    G->stream = NULL;

    mem_free(G);
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
