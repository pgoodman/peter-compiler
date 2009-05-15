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
PToken *token_alloc(PLexeme lexeme, PString *val, uint32_t line, uint32_t col) {
   

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

static void *L_generator_next(void *g) {
    static PToken *tokens[11];
    static int init = 0,
               pos = -1;

    if(0 == init) {
        init = 1;

        tokens[0] = token_alloc(P_LEXEME_PAREN_OPEN, string_alloc_char("(", 1), 1, 3);
        tokens[1] = token_alloc(P_LEXEME_NUMBER, string_alloc_char("1", 1), 1, 4);
        tokens[2] = token_alloc(P_LEXEME_ADD, string_alloc_char("+", 1), 1, 5);
        tokens[3] = token_alloc(P_LEXEME_NUMBER, string_alloc_char("2", 1), 1, 5);
        tokens[4] = token_alloc(P_LEXEME_PAREN_CLOSE, string_alloc_char(")", 1), 1, 7);
        tokens[5] = token_alloc(P_LEXEME_MULTIPLY, string_alloc_char("*", 1), 1, 2);
        tokens[6] = token_alloc(P_LEXEME_PAREN_OPEN, string_alloc_char("(", 1), 1, 3);
        tokens[7] = token_alloc(P_LEXEME_NUMBER, string_alloc_char("3", 1), 1, 4);
        tokens[8] = token_alloc(P_LEXEME_ADD, string_alloc_char("+", 1), 1, 5);
        tokens[9] = token_alloc(P_LEXEME_NUMBER, string_alloc_char("4", 1), 1, 5);
        tokens[10] = token_alloc(P_LEXEME_PAREN_CLOSE, string_alloc_char(")", 1), 1, 7);
    }

    if((++pos) < 11) {
        return tokens[pos];
    }

    return NULL;
}

static void L_generator_free(void *gen) {

}

PTokenGenerator *token_generator_alloc(void) {
    PTokenGenerator *G = generator_alloc(sizeof(PTokenGenerator));

    generator_init(G, &L_generator_next, &L_generator_free);

    return G;
}
