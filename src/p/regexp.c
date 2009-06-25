/*
 * regexp-parser.c
 *
 *  Created on: Jun 22, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-regexp.h>

static char buffer[3],
            curr_char;

static unsigned int line = 0,
                    column = 0,
                    in_char_class = 0,
                    first_char_in_class = 1,
                    got_char = 0;

static FILE *fp = NULL;

enum {
    L_START_GROUP,
    L_END_GROUP,
    L_START_CLASS,
    L_START_NEG_CLASS,
    L_END_CLASS,
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
    P_EXPR,
    P_CAT_EXPR,
    P_FACTOR,
    P_TERM,
    P_OR_EXPR,
    P_STRING,
    P_CHAR,
    P_CHAR_CLASS_STRING,
    P_OPT_CHAR_CLASS_STRING,
    P_CHAR_RANGE,
    P_CHARACTER_CLASS,
    P_NEGATED_CHARACTER_CLASS,
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
    buffer[0] = curr_char;

    if(in_char_class) {
        goto any_char;
    }

    switch(curr_char) {
        case '(': token->terminal = L_START_GROUP; break;
        case ')': token->terminal = L_END_GROUP; break;
        case '[':
            token->terminal = L_START_CLASS;
            in_char_class = 1;
            first_char_in_class = 1;

            curr_char = fgetc(fp);

            got_char = 1;

            ++column;
            if(feof(fp)) {
                return 0;
            }

            if(curr_char == '^') {
                buffer[0] = '[';
                buffer[1] = '^';
                buffer[2] = 0;
                token->terminal = L_START_NEG_CLASS;
            } else {
                ungetc(curr_char, fp);
            }

            break;
        case '+': token->terminal = L_POSITIVE_CLOSURE; break;
        case '*': token->terminal = L_KLEENE_CLOSURE; break;
        case '^': token->terminal = L_ANCHOR_START; break;
        case '$': token->terminal = L_ANCHOR_END; break;
        case '?': token->terminal = L_OPTIONAL; break;
        case '|': token->terminal = L_OR; break;
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
                        case '-': token->terminal = L_CHARACTER; break;
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
                    if(in_char_class && curr_char == '-') {
                        token->terminal = L_CHARACTER_RANGE;
                    } else {
all_chars:
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
        16, /* number of non-terminals */
        18, /* number of terminals */
        50, /* number of production phrases */
        80 /* number of phrase symbols */
    );

    /*
     * Machine
     *     : -<anchor_line_start> ^Expr -<anchor_line_end>
     *     : -<anchor_line_start> ^Expr
     *     : ^Expr -<anchor_line_end>
     *     : ^Expr
     *     ;
     */

    grammar_add_terminal_symbol(G, L_ANCHOR_START, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_ANCHOR_END, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_ANCHOR_START, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_ANCHOR_END, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_MACHINE);


    /*
     * OrExpr
     *     : ^Expr <or> CatExpr
     */

    grammar_add_non_terminal_symbol(G, P_EXPR, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_OR, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_CAT_EXPR, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_OR_EXPR);

    /*
     * Expr
     *     : OrExpr
     *     : CatExpr
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_OR_EXPR, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_CAT_EXPR, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_EXPR);

    /*
     * CatExpr
     *     : ^CatExpr ^Factor
     *     : ^Factor
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_CAT_EXPR, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_FACTOR, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_FACTOR, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_CAT_EXPR);

    /*
     * KleeneClosure : ^Term <kleene_closure> ;
     */

    grammar_add_non_terminal_symbol(G, P_TERM, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_KLEENE_CLOSURE, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_KLEENE_CLOSURE);

    /*
     * PositiveClosure : ^Term <positive_closure> ;
     */

    grammar_add_non_terminal_symbol(G, P_TERM, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_POSITIVE_CLOSURE, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_POSITIVE_CLOSURE);

    /*
     * OptionalTerm : ^Term <optional> ;
     */

    grammar_add_non_terminal_symbol(G, P_TERM, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_OPTIONAL, G_AUTO);
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

    grammar_add_non_terminal_symbol(G, P_KLEENE_CLOSURE, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_POSITIVE_CLOSURE, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_OPTIONAL_TERM, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_TERM, G_RAISE_CHILDREN);
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

    grammar_add_non_terminal_symbol(G, P_NEGATED_CHARACTER_CLASS, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_CHARACTER_CLASS, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_STRING, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_START_GROUP, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_EXPR, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_END_GROUP, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_TERM);

    /*
     * NegatedCharacterClass : <start_neg_class> ^CharClassString <end_class> ;
     */

    grammar_add_terminal_symbol(G, L_START_NEG_CLASS, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_CHAR_CLASS_STRING, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_END_CLASS, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_NEGATED_CHARACTER_CLASS);

    /*
     * CharRange
     *     : ^Char <character_range> ^Char
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_CHAR, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_CHARACTER_RANGE, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_CHAR, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_CHAR_RANGE);

    /*
     * OptCharClassString
     *     : ^CharClassString
     *     : <>
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_CHAR_CLASS_STRING, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_OPT_CHAR_CLASS_STRING);

    /*
     * CharClassString
     *     : -CharRange ^OptCharClassString
     *     : ^Char ^CharClassString
     *     : ^Char
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_CHAR_RANGE, G_NON_EXCLUDABLE);
    grammar_add_non_terminal_symbol(G, P_OPT_CHAR_CLASS_STRING, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_CHAR, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_CHAR_CLASS_STRING, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_CHAR, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_CHAR_CLASS_STRING);

    /*
     * CharacterClass : <start_class> ^CharClassString <end_class> ;
     */

    grammar_add_terminal_symbol(G, L_START_CLASS, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_CHAR_CLASS_STRING, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_END_CLASS, G_AUTO);
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

    grammar_add_non_terminal_symbol(G, P_CHAR, G_RAISE_CHILDREN);
    grammar_add_non_terminal_symbol(G, P_STRING, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_non_terminal_symbol(G, P_CHAR, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_STRING);

    return G;
}

PParseTree *parse_regexp(const char *file) {

    PParseTree *parse_tree = NULL;
    PScanner *scanner = (PScanner *) 0x1; /* fake scanner */
    PGrammar *grammar = regex_grammar();

    char terminal_names[18][40] = {
        "start_group",
        "end_group",
        "start_class",
        "start_neg_class",
        "end_class",
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

    char production_names[16][40] = {
        "Machine",
        "Expr",
        "Concatenate",
        "Factor",
        "Term",
        "OrExpr",
        "String",
        "Char",
        "CharClassString",
        "OptCharClassString",
        "CharRange",
        "CharClass",
        "NegatedCharClass",
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
