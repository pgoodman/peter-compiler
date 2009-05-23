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
static void L_generator_free(PTokenGenerator *gen) {
    assert_not_null(gen);
    assert_not_null(gen->stream);

    file_free(gen->stream);
    gen->stream = NULL;

    mem_free(gen);
}

/**
 * Allocate a new token generator. The next function is the function that
 * actually lexes input. This allows the token generator to be completely
 * general.
 */
PTokenGenerator *token_generator_alloc(PFileInputStream *stream,
                                       PFunction gen_next_fnc) {
    PTokenGenerator *G = NULL;

    assert_not_null(stream);
    assert_not_null(generator_next);

    G = generator_alloc(sizeof(PTokenGenerator));

    generator_init(
        G,
        gen_next_fnc,
        (PDelegate) &L_generator_free
    );

    G->stream = stream;
    G->column = 1;
    G->line = 1;
    G->start_char = -1;

    return G;
}
