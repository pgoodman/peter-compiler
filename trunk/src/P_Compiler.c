/*
 ============================================================================
 Name        : P_Compiler.c
 Author      : Peter Goodman
 Version     :
 ============================================================================
 */

#include <std-include.h>
#include <p-lexer.h>
#include <p-parser.h>

unsigned int __st_depth = 0;

PParseTree *Additive($$ PParseTree *x) { $H
    printf("In additive.\n");
    return_with NULL;
}

PParseTree *Multitive($$ PParseTree *x) { $H
    printf("In multitive.\n");
    return_with NULL;
}

PParseTree *Primary($$ PParseTree *x) { $H
    printf("In primary.\n");
    return_with NULL;
}

PParseTree *Decimal($$ PParseTree *x) { $H
    printf("In decimal.\n");
    return_with NULL;
}

int main() { $MH

    PParser *P = parser_alloc($A);

    /*
     * Additive  <-- Multitive '+' Additive
     *            /  Multitive
     *
     * Multitive <-- Primary '*' Multitive
     *            /  Primary
     *
     * Primary   <-- '(' Additive ')'
     *            /  *number*
     */

    parser_add_production($$A P, &Additive, 2,
        parser_rule_sequence($$A 3,
            parser_rewrite_function($$A P, &Multitive),
            parser_rewrite_token($$A P, P_LEXEME_ADD),
            parser_rewrite_function($$A P, &Additive)
        ),
        parser_rule_sequence($$A 1,
            parser_rewrite_function($$A P, &Multitive)
        )
    );

    parser_add_production($$A P, &Multitive, 2,
        parser_rule_sequence($$A 3,
            parser_rewrite_function($$A P, &Primary),
            parser_rewrite_token($$A P, P_LEXEME_MULTIPLY),
            parser_rewrite_function($$A P, &Multitive)
        ),
        parser_rule_sequence($$A 1,
            parser_rewrite_function($$A P, &Primary)
        )
    );

    parser_add_production($$A P, &Primary, 2,
        parser_rule_sequence($$A 3,
            parser_rewrite_token($$A P, P_LEXEME_PAREN_OPEN),
            parser_rewrite_function($$A P, &Additive),
            parser_rewrite_token($$A P, P_LEXEME_PAREN_CLOSE)
        ),
        parser_rule_sequence($$A 1,
            parser_rewrite_token($$A P, P_LEXEME_NUMBER)
        )
    );

    return 0;
}
