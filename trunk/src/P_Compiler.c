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
#include <adt-dict.h>

#if defined(P_DEBUG_PRINT_TRACE) && P_DEBUG_PRINT_TRACE == 1
unsigned int __st_depth = 0;
#endif

void Additive(PProductionTree *T, PDictionary *garbage) {
    printf("In additive.\n");
    /*
    switch(T->rule) {
    case 1:

    case 2:

    }*/
}

void Multitive(PProductionTree *T, PDictionary *garbage) {
    printf("In multitive.\n");
    /*PParseTree *left = NULL,
                right = NULL;
    if(T->rule == 1) {
        left = tree_get_branch(tree_get_branch(T, 0), 0);
        right = tree_get_branch(tree_get_branch(T, 2), 0);

        dict_unset(garbage, left, &delegate_do_nothing);
        dict_unset(garbage, right, &delegate_do_nothing);

        dict_clear(T);

        dict_add_branch(left, right);
    } else {
        left = tree_get_branch(tree_get_branch(T, 0), 0);
    }*/
}

void Primary(PProductionTree *T, PDictionary *garbage) {
    printf("In primary.\n");
    /*
    PParseTree *inner;

    if(T->rule == 1) {
        inner = tree_get_branch(T, 1);
        tree_clear(T);
        tree_add_branch(T, inner);

        dict_unset(garbage, inner, &delegate_do_nothing);
    }*/
}

int main() {

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

    PTreeGenerator *gen;
    PParseTree *curr;
    PParserFunc f;

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

    parser_parse_tokens(P, G);

    /*printf("traversal of the parse tree:\n");
    gen = tree_generator_alloc(T, TREE_TRAVERSE_POSTORDER );
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
    generator_free(gen);*/

    printf("done.\n");

    return 0;
}
