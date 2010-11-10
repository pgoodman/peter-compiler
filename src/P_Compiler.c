/*
 ============================================================================
 Name        : P_Compiler.c
 Author      : Peter Goodman
 Version     :
 ============================================================================
 */

#include <std-include.h>

#if 1
#include <pgen-gen.h>


#include <c-grammar.h>
#include <c-lexer.h>
#include <p-scanner.h>

#if 0
#include "gen/lang_grammar.h"
#include "gen/lang_lexer.h"
#endif

#include <p-regexp.h>
#endif

#include "adt-nfa.h"

static int print_options(void) {
    printf(
        "Regular expression to finite automata tool:\n"
        "\tArguments:\n"
        "\t\t -nfa <regex>\n"
        "\t\t -dfa <regex>\n"
        "\t\t -mdfa <regex>\n"
        "\tOptions:\n"
        "\t\t-label-subsets\n"
        "\t\t-latex\n\n"
    );
    return 0;
}

/* -------------------------------------------------------------------------- */

int main(int argc, char *argv[]) {

    PNFA *nfa = nfa_alloc();
    PNFA *dfa = nfa;
    PSet *set = set_alloc();
    int start = nfa_add_state(nfa);
    int i;
    int seen_tool = 0;
    int out_dot = 1;
    PScanner *scanner = scanner_alloc();
    PGrammar *grammar = regexp_grammar();

    if(2 >= argc) {
        return print_options();
    }

    for(i = 1; i < argc; ++i) {
        if(0 == strcmp("-label-subsets", argv[i])) {
            nfa_label_states(1, 1);
        } else if(0 == strcmp("-dfa", argv[i])) {
            seen_tool = 1;
        } else if(0 == strcmp("-mdfa", argv[i])) {
            seen_tool = 1;
        } else if(0 == strcmp("-nfa", argv[i])) {
            seen_tool = 1;
        } else if(0 == strcmp("-latex", argv[i])) {
            out_dot = 0;
        }
    }

    if(!seen_tool) {
        return print_options();
    }

    for(i = 1; i < argc; ++i) {

        if(0 == strcmp("-label-subsets", argv[i])
        || 0 == strcmp("-latex", argv[i])) {
            continue;
        } else if((i + 1) < argc && '-' == argv[i][0]
               && ('d' == argv[i][1] || 'm' == argv[i][1] || 'n' == argv[i][1])){

            regexp_parse(
                grammar,
                scanner,
                nfa,
                (unsigned char *) argv[i + 1],
                start,
                0 /* we don't care what the conclusion is */
            );

            if(0 == strcmp("-dfa", argv[i])) {
                dfa = nfa_to_dfa(nfa, set);
                nfa_free(nfa);
            } else if(0 == strcmp("-mdfa", argv[i])) {
                dfa = nfa_to_mdfa(nfa, set);
                nfa_free(nfa);
            }

            ++i;
        }
    }

    if(out_dot) {
        nfa_print_dot(dfa);
    } else {
        nfa_print_latex(dfa);
    }
    nfa_free(dfa);
    set_free(set);
    scanner_free(scanner);
    grammar_free(grammar);

#if 0
    /*

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

    parser_gen(
        "src/grammars/lang.g",
        "lang_grammar",
        "src/gen/lang_grammar.h",
        "lang_lexer",
        "src/gen/lang_lexer.h",
        "lang"
    );

/*
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
*/
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
#endif
    return 0;
}
