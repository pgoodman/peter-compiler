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

    nfa_add_state(nfa);

    printf("parsing first... \n");

    regexp_parse(
        grammar,
        scanner,
        nfa,
        (unsigned char *) "[>\\-+]+",
        0,
        1
    );

    printf("parsing second... \n");

    regexp_parse(
        grammar,
        scanner,
        nfa,
        (unsigned char *) "->",
        0,
        2
    );

    dfa = nfa_to_dfa(nfa);

    nfa_free(nfa);
    nfa_free(dfa);

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

#if defined(P_DEBUG) && P_DEBUG == 1 && defined(P_DEBUG_MEM) && P_DEBUG_MEM == 1
    printf("num unfreed pointers: %ld\n", mem_num_allocated_pointers());
#endif

    return 0;
}
