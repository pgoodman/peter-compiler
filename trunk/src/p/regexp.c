/*
 * regexp-parser.c
 *
 *  Created on: Jun 22, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-regexp.h>

static char buffer[2],
            curr_char;

static unsigned int line = 0,
                    column = 0,
                    in_char_class = 0,
                    first_char_in_class = 1;

static FILE *fp = NULL;

enum {
    L_START_GROUP,
    L_END_GROUP,
    L_START_CLASS,
    L_END_CLASS,
    L_NEGATE_CLASS,
    L_POSITIVE_CLOSURE,
    L_KLEENE_CLOSURE,
    L_OPTIONAL,
    L_CARROT,
    L_ANCHOR_START,
    L_ANCHOR_END,
    L_OR,
    L_CHARACTER_RANGE,
    L_CHARACTER,
    L_SPACE,
    L_NEW_LINE,
    L_CARRIAGE_RETURN,
    L_TAB
};

enum {
    P_MACHINE,
    P_EXPRS,
    P_EXPR,
    P_CAT_EXPR,
    P_FACTOR,
    P_TERM,

    P_OR_EXPR,

    P_STRING,
    P_CHAR,

    P_CHARACTER_CLASS,
    P_NEGATED_CHARACTER_CLASS,

    P_START_ANCHOR,
    P_END_ANCHOR,
    P_NO_ANCHOR,

    P_KLEENE_CLOSURE,
    P_POSITIVE_CLOSURE,
    P_OPTIONAL_TERM
};

static int R_get_token(PScanner *scanner, PToken *token) {

    do {
        ++column;
        curr_char = fgetc(fp);

        if(curr_char == '\n') {
            ++line;
            column = 0;
        }
    } while(isspace(curr_char));

    if(feof(fp)) {
        return 0;
    }

    token->lexeme_length = 1;
    token->line = line;
    token->column = column;
    token->lexeme = buffer;

    buffer[1] = 0;

    if(in_char_class) {
        goto any_char;
    }

    buffer[0] = curr_char;

    switch(curr_char) {
        case '(': token->terminal = L_START_CLASS; break;
        case ')': token->terminal = L_END_CLASS; break;
        case '[':
            token->terminal = L_START_CLASS;
            in_char_class = 1;
            first_char_in_class = 1;
            break;
        case '+': token->terminal = L_POSITIVE_CLOSURE; break;
        case '*': token->terminal = L_KLEENE_CLOSURE; break;
        case '^': token->terminal = L_ANCHOR_START; break;
        case '$': token->terminal = L_ANCHOR_END; break;
        case '?': token->terminal = L_OPTIONAL; break;
        case '|': token->terminal = L_OR; break;
        case '-': token->terminal = L_CHARACTER_RANGE; break;
        default:
any_char:
            switch(curr_char) {

                case ']':
                    token->terminal = L_END_CLASS; in_char_class = 0;
                    break;

                case '\\':
                    curr_char = fgetc(fp);
                    ++column;
                    if(feof(fp)) {
                        return 0;
                    }

                    buffer[0] = curr_char;

                    switch(curr_char) {
                        case 'n': token->terminal = L_NEW_LINE; break;
                        case 's': token->terminal = L_SPACE; break;
                        case 't': token->terminal = L_TAB; break;
                        case 'r': token->terminal = L_CARRIAGE_RETURN; break;
                        default:
                            if(isspace(curr_char)) {
                                printf(
                                    "Scanner Error: a space cannot follow a "
                                    "backslash.\n"
                                );
                                exit(1);
                            }

                            goto all_chars;
                    }
                    break;

                default:
all_chars:
                    if(first_char_in_class && curr_char == '^') {
                        token->terminal = L_NEGATE_CLASS;
                    } else {
                        token->terminal = L_CHARACTER;
                    }
            }

            first_char_in_class = 0;
    }

    return 1;
}

static PGrammar *regex_grammar(void) {
    PGrammar *G = grammar_alloc(
        P_MACHINE, /* production to start matching with */
        17, /* number of non-terminals */
        18, /* number of terminals */
        33, /* number of production phrases */
        60 /* number of phrase symbols */
    );

    /*
     * Machine
     *     : <anchor_line_start> ^Expr <anchor_line_end>
     *     : <anchor_line_start> ^Expr
     *     : ^Expr <anchor_line_end>
     *     : ^Expr
     *     : <>
     *     ;
     */

    grammar_add_terminal_symbol(G, L_ANCHOR_START, G_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_ANCHOR_END, G_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_ANCHOR_START, G_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_ANCHOR_END, G_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_MACHINE);

    /*
     * Exprs
     *     : OrExpr
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_OR_EXPR, G_NON_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_START_ANCHOR);

    grammar_add_non_terminal_symbol(G, P_START_ANCHOR, G_NON_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_EXPRS);


    /*
     * OrExpr : ^Expr <or> ^CatExpr ;
     */

    grammar_add_non_terminal_symbol(G, P_EXPR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_OR, G_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_CAT_EXPR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_OR_EXPR);

    /*
     * Expr
     *     : -OrExpr
     *     : ^CatExpr
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_OR_EXPR, G_NON_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_CAT_EXPR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_EXPR);

    /*
     * CatExpr
     *     : ^CatExpr Factor
     *     : Factor
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_CAT_EXPR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_FACTOR, G_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_FACTOR, G_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_CAT_EXPR);

    /*
     * KleeneClosure : ^Term <kleene_closure> ;
     */

    grammar_add_non_terminal_symbol(G, P_TERM, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_KLEENE_CLOSURE, G_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_KLEENE_CLOSURE);

    /*
     * PositiveClosure : ^Term <positive_closure> ;
     */

    grammar_add_non_terminal_symbol(G, P_TERM, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_POSITIVE_CLOSURE, G_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_POSITIVE_CLOSURE);

    /*
     * OptionalTerm : ^Term <optional> ;
     */

    grammar_add_non_terminal_symbol(G, P_TERM, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_OPTIONAL, G_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_OPTIONAL_TERM);

    /*
     * Factor
     *     : -KleeneClosure
     *     : -PositiveClosure
     *     : -OptionalTerm
     *     : ^Term
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_KLEENE_CLOSURE, G_NON_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_POSITIVE_CLOSURE, G_NON_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_OPTIONAL_TERM, G_NON_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_TERM, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_FACTOR);

    /*
     * Term
     *     : -NegatedCharacterClass
     *     : -CharacterClass
     *     : -String
     *     : <start_group> ^Expr <end_group>
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_NEGATED_CHARACTER_CLASS, G_NON_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_CHARACTER_CLASS, G_NON_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_STRING, G_NON_EXCLUDABLE, G_AUTO_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_START_GROUP, G_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_END_GROUP, G_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_TERM);

    /*
     * NegatedCharacterClass : <start_class> <negate_class> ^String <end_class> ;
     */

    grammar_add_terminal_symbol(G, L_START_CLASS, G_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_NEGATE_CLASS, G_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_STRING, G_NON_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_END_CLASS, G_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_NEGATED_CHARACTER_CLASS);

    /*
     * CharacterClass : <start_class> ^String <end_class> ;
     */

    grammar_add_terminal_symbol(G, L_START_CLASS, G_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_STRING, G_NON_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_END_CLASS, G_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_CHARACTER_CLASS);

    /*
     * Char
     *     : -<characher>
     *     : -<space>
     *     : -<tab>
     *     : -<new_line>
     *     : -<carriage_return>
     *     ;
     */

    grammar_add_terminal_symbol(G, L_CHARACTER, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_SPACE, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_TAB, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_NEW_LINE, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_CARRIAGE_RETURN, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_CHAR);

    /*
     * String
     *     : ^Char ^String
     *     : ^Char
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_CHAR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_STRING, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_CHAR, G_EXCLUDABLE, G_MANUAL_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_STRING);

    return G;
}

PParseTree *parse_regexp(const char *file) {

    PParseTree *parse_tree = NULL;
    PScanner *scanner = (PScanner *) 0x1; /* fake scanner */
    PGrammar *grammar = regex_grammar();

    char terminal_names[20][40] = {
        "start_group",
        "end_group",
        "start_class",
        "end_class",
        "negate_class",
        "positive_closure",
        "kleene_closure",
        "optional",
        "carrot",
        "anchor_start",
        "anchor_end",
        "or",
        "character_range",
        "character",
        "space",
        "new_line",
        "carriage_return",
        "tab"
    };

    char production_names[20][40] = {
        "Machine",
        "DummyExpr",
        "Expr",
        "CatExpr",
        "Factor",
        "Term",
        "OrExpr",
        "String",
        "Char",
        "CharClass",
        "NegatedCharClass",
        "DummyExpr2",
        "EndAnchor",
        "NoAnchor",
        "KleeneClosure",
        "PositiveClosure",
        "OptionalTerm"
    };

    if(is_not_null(fp = fopen(file, "r"))) {

        parse_tree = parse_tokens(
            grammar,
            scanner,
            (PScannerFunction *) &R_get_token
        );

        printf("Printing tree: \n\n");

        parse_tree_print_dot(
            parse_tree,
            production_names,
            terminal_names
        );

        printf("\nFreeing memory.. \n");

        parse_tree_free(parse_tree);
        grammar_free(grammar);

        fclose(fp);
    } else {
        printf("Error: Could not open file '%s'.", file);
    }

    return parse_tree;
}
