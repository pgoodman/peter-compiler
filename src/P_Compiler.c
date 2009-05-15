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


#if defined(P_DEBUG_PRINT_TRACE) && P_DEBUG_PRINT_TRACE == 1
unsigned int __st_depth = 0;
#endif

PParseTree *Additive(PParseTree *x) { $H
    printf("In additive.\n");
    return_with NULL;
}

PParseTree *Multitive(PParseTree *x) { $H
    printf("In multitive.\n");
    return_with NULL;
}

PParseTree *Primary(PParseTree *x) { $H
    printf("In primary.\n");
    return_with NULL;
}

PParseTree *Decimal(PParseTree *x) { $H
    printf("In decimal.\n");
    return_with NULL;
}

int main() { $MH

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

    PParser *P = parser_alloc(&Additive);
    PParseTree *T = NULL;
    PTokenGenerator *G = token_generator_alloc();

    parser_add_production(P, &Additive, 2,
        parser_rule_sequence(3,
            parser_rewrite_function(P, &Multitive),
            parser_rewrite_token(P, P_LEXEME_ADD),
            parser_rewrite_function(P, &Additive)
        ),
        parser_rule_sequence(1,
            parser_rewrite_function(P, &Multitive)
        )
    );

    parser_add_production(P, &Multitive, 2,
        parser_rule_sequence(3,
            parser_rewrite_function(P, &Primary),
            parser_rewrite_token(P, P_LEXEME_MULTIPLY),
            parser_rewrite_function(P, &Multitive)
        ),
        parser_rule_sequence(1,
            parser_rewrite_function(P, &Primary)
        )
    );

    parser_add_production(P, &Primary, 2,
        parser_rule_sequence(3,
            parser_rewrite_token(P, P_LEXEME_PAREN_OPEN),
            parser_rewrite_function(P, &Additive),
            parser_rewrite_token(P, P_LEXEME_PAREN_CLOSE)
        ),
        parser_rule_sequence(1,
            parser_rewrite_token(P, P_LEXEME_NUMBER)
        )
    );

    T = parser_parse_tokens(P, G);

    printf("bottom-up traversal of the parse tree:\n");
    PTreeGenerator *gen = tree_generator_alloc(T, TREE_TRAVERSE_POSTORDER );
    PParseTree *curr;
    PParserFunc f;
    int n;
    while(generator_next(gen)) {
        curr = generator_current(gen);
        if(P_PARSE_TREE_PRODUCTION == curr->type) {
            f = ((PProductionTree *) curr)->production;
            n = ((PProductionTree *) curr)->rule;
            if(f == &Additive)
                printf("Additive %d\n", n);
            else if(f == &Multitive)
                printf("Multitive %d\n", n);
            else
                printf("Primary %d\n", n);
        } else {
            printf("%s\n", ((PTerminalTree *) curr)->token->val->str);
        }
    }
    generator_free(gen);

    printf("done.\n");

    return 0;
}
