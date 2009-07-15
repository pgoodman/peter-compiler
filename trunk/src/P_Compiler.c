/*
 ============================================================================
 Name        : P_Compiler.c
 Author      : Peter Goodman
 Version     :
 ============================================================================
 */

#include <std-include.h>

#include <pgen-gen.h>



#include <c-grammar.h>
#include <c-lexer.h>
#include <p-scanner.h>



#include <p-regexp.h>


/* -------------------------------------------------------------------------- */

int main(void) {
    /*
    PScanner *scanner = scanner_alloc();
    PGrammar *grammar = regexp_grammar();
    PNFA *nfa = nfa_alloc(),
         *dfa;
    PSet *set = set_alloc();

    nfa_add_state(nfa);

    printf("parsing fourth... \n");

    set_add_elm(set, regexp_parse_cat(
        grammar,
        scanner,
        nfa,
        (unsigned char *) ":",
        0,
        3
    ));

    printf("parsing second... \n");

    set_add_elm(set, regexp_parse_cat(
        grammar,
        scanner,
        nfa,
        (unsigned char *) "abc",
        0,
        2
    ));

    printf("parsing third... \n");

    regexp_parse(
        grammar,
        scanner,
        nfa,
        (unsigned char *) "[A-C]*",
        0,
        3
    );

    printf("parsing first... \n");

    regexp_parse(
        grammar,
        scanner,
        nfa,
        (unsigned char *) "[a-c][a-c1-3]*",
        0,
        1
    );

    nfa_print_dot(nfa);

    dfa = nfa_to_dfa(nfa, set);

    printf("\n\n");

    nfa_print_dot(dfa);

    nfa_free(nfa);
    nfa_free(dfa);

    set_free(set);
    grammar_free(grammar);
    scanner_free(scanner);
    */
    /*
    parser_gen(
        "src/grammars/lang.g",
        "lang_grammar",
        "src/gen/lang_grammar.h",
        "lang_lexer",
        "src/gen/lang_lexer.h",
        "lang"
    );
    */

    PScanner *scanner = scanner_alloc();
    PGrammar *grammar = lang_grammar();

    if(scanner_use_file(scanner, "src/gen/test.ln")) {
        scanner_flush(scanner, 1);
        parse_tokens(
            grammar,
            scanner,
            (PScannerFunc *) &lang_lexer,
            NULL
        );
    }

    scanner_free(scanner);
    grammar_free(grammar);

    /*
    PScanner *scanner = scanner_alloc();
    PGrammar *grammar = lang_grammar();

    if(scanner_use_string(scanner, (unsigned char *) "defun")) {
        scanner_flush(scanner, 1);
        printf("result is %d \n", lang_lexer(scanner));
    }

    scanner_free(scanner);
    grammar_free(grammar);
    */
#if defined(P_DEBUG) && P_DEBUG == 1 && defined(P_DEBUG_MEM) && P_DEBUG_MEM == 1
    printf("num unfreed pointers: %ld\n", mem_num_allocated_pointers());
#endif

    return 0;
}
