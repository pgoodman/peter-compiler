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
    L_TAB,
    L_ANY_CHAR
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

typedef struct PThompsonsConstruction {
    PNFA *nfa;
    unsigned int state_stack[256];
    int top_state;
} PThompsonsConstruction;

static void Machine(PThompsonsConstruction *thompson,
                 unsigned char phrase,
                 unsigned int num_branches,
                 PParseTree *branches[]) {

    unsigned int start, end;
    PT_Terminal *term;
    PParseTree *tree;

    switch(num_branches) {
        /* ... */
        case 1:
            if(branches[0]->type != PT_NON_TERMINAL) {
                return;
            }
            start = thompson->state_stack[thompson->top_state--];
            end = thompson->state_stack[thompson->top_state--];
            break;

        case 2:
            /* ^ ... */
            if(branches[0]->type == PT_TERMINAL) {
                start = nfa_add_state(thompson->nfa);

                /* ^ ... */
                if(branches[1]->type != PT_TERMINAL) {
                    nfa_add_value_transition(
                         thompson->nfa,
                         start,
                         thompson->state_stack[thompson->top_state--],
                         '\n'
                    );
                    end = thompson->state_stack[thompson->top_state--];

                /* ^$ */
                } else {
                    end = nfa_add_state(thompson->nfa);
                    nfa_add_value_transition(thompson->nfa, start, end, '\n');
                }
            /* ... $ */
            } else {
                start = thompson->state_stack[thompson->top_state--];
                end = nfa_add_state(thompson->nfa);
                nfa_add_value_transition(
                     thompson->nfa,
                     thompson->state_stack[thompson->top_state--],
                     end,
                     '\n'
                );
            }
            break;

        /* ^ ... $ */
        case 3:
            start = nfa_add_state(thompson->nfa);
            end = nfa_add_state(thompson->nfa);
            nfa_add_value_transition(
                 thompson->nfa,
                 start,
                 thompson->state_stack[thompson->top_state--],
                 '\n'
            );
            nfa_add_value_transition(
                 thompson->nfa,
                 thompson->state_stack[thompson->top_state--],
                 end,
                 '\n'
            );
            break;
        default:
            start = nfa_add_state(thompson->nfa);
            end = start;
            break;
    }

    nfa_change_start_state(thompson->nfa, start);
    nfa_add_accepting_state(thompson->nfa, end);
}

static void CatExpr(PThompsonsConstruction *thompson,
                 unsigned char phrase,
                 unsigned int num_branches,
                 PParseTree *branches[]) {

    unsigned int start, prev, end, inter_start, inter_end;
    PT_Terminal *character;

    end = thompson->state_stack[thompson->top_state - 1];

    if(num_branches > 1) {
        for(; --num_branches; ) {
            inter_start = thompson->state_stack[thompson->top_state];
            inter_end = thompson->state_stack[thompson->top_state - 3];

            nfa_add_epsilon_transition(
                thompson->nfa,
                inter_end,
                inter_start
            );

            thompson->top_state -= 2;
        }
    }

    printf("bottom of stack: %d \n", thompson->top_state - 1);

    thompson->state_stack[thompson->top_state - 1] = end;
}

static void OrExpr(PThompsonsConstruction *thompson,
                 unsigned char phrase,
                 unsigned int num_branches,
                 PParseTree *branches[]) {

    unsigned int a1_start, a1_end, a2_start, a2_end, start, end;

    a1_start = thompson->state_stack[thompson->top_state--];
    a1_end = thompson->state_stack[thompson->top_state--];
    a2_start = thompson->state_stack[thompson->top_state--];
    a2_end = thompson->state_stack[thompson->top_state--];

    start = nfa_add_state(thompson->nfa);
    end = nfa_add_state(thompson->nfa);

    nfa_add_epsilon_transition(thompson->nfa, start, a1_start);
    nfa_add_epsilon_transition(thompson->nfa, start, a2_start);

    nfa_add_epsilon_transition(thompson->nfa, a1_end, end);
    nfa_add_epsilon_transition(thompson->nfa, a2_end, end);

    thompson->state_stack[++thompson->top_state] = end;
    thompson->state_stack[++thompson->top_state] = start;
}

static void String(PThompsonsConstruction *thompson,
                 unsigned char phrase,
                 unsigned int num_branches,
                 PParseTree *branches[]) {

    unsigned int start, prev, end, i;
    PT_Terminal *character;

    start = nfa_add_state(thompson->nfa);
    prev = start;

    for(i = 0; i < num_branches; ++i) {
        character = (PT_Terminal *) branches[i];
        end = nfa_add_state(thompson->nfa);
        nfa_add_value_transition(
             thompson->nfa,
             prev,
             end,
             character->lexeme->str[0]
        );
        prev = end;
    }

    thompson->state_stack[++thompson->top_state] = end;
    thompson->state_stack[++thompson->top_state] = start;
}

typedef void (R_set_elm_fnc_t)(PSet *, unsigned int);

static void R_char_class(PThompsonsConstruction *thompson,
                         unsigned char phrase,
                         unsigned int num_branches,
                         PParseTree *branches[],
                         PSet *set,
                         R_set_elm_fnc_t *fnc) {

    unsigned int start, end, i;
    unsigned char range_start, range_end;
    PT_NonTerminal *range;

    for(i = 0; i < num_branches; ++i) {
        if(branches[i]->type == PT_NON_TERMINAL) {
            range = (PT_NonTerminal *) branches[i];
            range_start = ((PT_Terminal *) tree_get_branch(range, 0))->lexeme->str[0];
            range_end = ((PT_Terminal *) tree_get_branch(range, 1))->lexeme->str[0];

            for(; range_start <= range_end; ++range_start) {
                fnc(set, range_start);
            }
        } else {
            fnc(set,((PT_Terminal *) branches[i])->lexeme->str[0]);
        }
    }

    start = nfa_add_state(thompson->nfa);
    end = nfa_add_state(thompson->nfa);
    nfa_add_set_transition(thompson->nfa, start, end, set);

    thompson->state_stack[++thompson->top_state] = end;
    thompson->state_stack[++thompson->top_state] = start;
}

static void CharClass(PThompsonsConstruction *thompson,
                 unsigned char phrase,
                 unsigned int num_branches,
                 PParseTree *branches[]) {
    R_char_class(
        thompson,
        phrase,
        num_branches,
        branches,
        set_alloc(),
        &set_add_elm
    );
}

static void NegatedCharClass(PThompsonsConstruction *thompson,
                 unsigned char phrase,
                 unsigned int num_branches,
                 PParseTree *branches[]) {
    R_char_class(
        thompson,
        phrase,
        num_branches,
        branches,
        set_alloc_inverted(),
        &set_remove_elm
    );
}

static void KleeneClosure(PThompsonsConstruction *thompson,
                 unsigned char phrase,
                 unsigned int num_branches,
                 PParseTree *branches[]) {
    unsigned int c_start, c_end, start, end, loop_start, loop_end;

    loop_start = thompson->state_stack[thompson->top_state--];
    loop_end = thompson->state_stack[thompson->top_state--];

    start = nfa_add_state(thompson->nfa);
    end = nfa_add_state(thompson->nfa);

    nfa_add_epsilon_transition(thompson->nfa, start, loop_start);
    nfa_add_epsilon_transition(thompson->nfa, start, end);
    nfa_add_epsilon_transition(thompson->nfa, loop_end, end);
    nfa_add_epsilon_transition(thompson->nfa, loop_end, loop_start);

    thompson->state_stack[++thompson->top_state] = end;
    thompson->state_stack[++thompson->top_state] = start;
}

static void PositiveClosure(PThompsonsConstruction *thompson,
                 unsigned char phrase,
                 unsigned int num_branches,
                 PParseTree *branches[]) {
    unsigned int c_start, c_end, start, end, loop_start, loop_end;

    loop_start = thompson->state_stack[thompson->top_state--];
    loop_end = thompson->state_stack[thompson->top_state--];

    start = nfa_add_state(thompson->nfa);
    end = nfa_add_state(thompson->nfa);

    nfa_add_epsilon_transition(thompson->nfa, start, loop_start);
    nfa_add_epsilon_transition(thompson->nfa, loop_end, end);
    nfa_add_epsilon_transition(thompson->nfa, loop_end, loop_start);

    thompson->state_stack[++thompson->top_state] = end;
    thompson->state_stack[++thompson->top_state] = start;
}

static void OptionalTerm(PThompsonsConstruction *thompson,
                 unsigned char phrase,
                 unsigned int num_branches,
                 PParseTree *branches[]) {
unsigned int c_start, c_end, start, end, loop_start, loop_end;

    loop_start = thompson->state_stack[thompson->top_state--];
    loop_end = thompson->state_stack[thompson->top_state--];

    start = nfa_add_state(thompson->nfa);
    end = nfa_add_state(thompson->nfa);

    nfa_add_epsilon_transition(thompson->nfa, start, loop_start);
    nfa_add_epsilon_transition(thompson->nfa, start, end);
    nfa_add_epsilon_transition(thompson->nfa, loop_end, end);

    thompson->state_stack[++thompson->top_state] = end;
    thompson->state_stack[++thompson->top_state] = start;
}

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
        case '.': token->terminal = L_ANY_CHAR; break;
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
        19, /* number of terminals */
        50, /* number of production phrases */
        80 /* number of phrase symbols */
    );

    /*
     * Machine
     *     : -<anchor_line_start> -<anchor_line_end>
     *     : -<anchor_line_start> ^Expr -<anchor_line_end>
     *     : -<anchor_line_start> ^Expr
     *     : ^Expr -<anchor_line_end>
     *     : ^Expr
     *     : <>
     *     ;
     */

    grammar_add_terminal_symbol(G, L_ANCHOR_START, G_NON_EXCLUDABLE);
    grammar_add_terminal_symbol(G, L_ANCHOR_END, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
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
    grammar_add_epsilon_symbol(G, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_MACHINE, (G_ProductionRuleFunc *) &Machine);


    /*
     * OrExpr
     *     : ^Expr <or> CatExpr
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_EXPR, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_OR, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_CAT_EXPR, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_OR_EXPR, (G_ProductionRuleFunc *) &OrExpr);

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
    grammar_add_production_rule(G, P_EXPR, &grammar_null_action);

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
    grammar_add_production_rule(G, P_CAT_EXPR, (G_ProductionRuleFunc *) &CatExpr);

    /*
     * KleeneClosure : ^Term <kleene_closure> ;
     */

    grammar_add_non_terminal_symbol(G, P_TERM, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_KLEENE_CLOSURE, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_KLEENE_CLOSURE, (G_ProductionRuleFunc *) &KleeneClosure);

    /*
     * PositiveClosure : ^Term <positive_closure> ;
     */

    grammar_add_non_terminal_symbol(G, P_TERM, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_POSITIVE_CLOSURE, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_POSITIVE_CLOSURE, (G_ProductionRuleFunc *) &PositiveClosure);

    /*
     * OptionalTerm : ^Term <optional> ;
     */

    grammar_add_non_terminal_symbol(G, P_TERM, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_OPTIONAL, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_OPTIONAL_TERM, (G_ProductionRuleFunc *) &OptionalTerm);

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
    grammar_add_production_rule(G, P_FACTOR, &grammar_null_action);

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
    grammar_add_production_rule(G, P_TERM, &grammar_null_action);

    /*
     * NegatedCharacterClass : <start_neg_class> ^CharClassString <end_class> ;
     */

    grammar_add_terminal_symbol(G, L_START_NEG_CLASS, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_CHAR_CLASS_STRING, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_END_CLASS, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_NEGATED_CHARACTER_CLASS, (G_ProductionRuleFunc *) &NegatedCharClass);

    /*
     * CharRange
     *     : ^Char <character_range> ^Char
     *     ;
     */

    grammar_add_non_terminal_symbol(G, P_CHAR, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_CHARACTER_RANGE, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_CHAR, G_RAISE_CHILDREN);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_CHAR_RANGE, &grammar_null_action);

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
    grammar_add_production_rule(G, P_OPT_CHAR_CLASS_STRING, &grammar_null_action);

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
    grammar_add_production_rule(G, P_CHAR_CLASS_STRING, &grammar_null_action);

    /*
     * CharacterClass : <start_class> ^CharClassString <end_class> ;
     */

    grammar_add_terminal_symbol(G, L_START_CLASS, G_AUTO);
    grammar_add_non_terminal_symbol(G, P_CHAR_CLASS_STRING, G_RAISE_CHILDREN);
    grammar_add_terminal_symbol(G, L_END_CLASS, G_AUTO);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_CHARACTER_CLASS, (G_ProductionRuleFunc *) &CharClass);

    /*
     * Char
     *     : -<characher>
     *     : -<any_char>
     *     : -<space>
     *     : -<tab>
     *     : -<new_line>
     *     : -<carriage_return>
     *     ;
     */

    grammar_add_terminal_symbol(G, L_CHARACTER, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_ANY_CHAR, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_SPACE, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_TAB, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_NEW_LINE, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_terminal_symbol(G, L_CARRIAGE_RETURN, G_NON_EXCLUDABLE);
    grammar_add_phrase(G);
    grammar_add_production_rule(G, P_CHAR, &grammar_null_action);

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
    grammar_add_production_rule(G, P_STRING, (G_ProductionRuleFunc *) &String);

    return G;
}

void parse_regexp(const char *file) {

    PScanner *scanner = (PScanner *) 0x1; /* fake scanner */
    PGrammar *grammar = regex_grammar();
    PThompsonsConstruction thom, *thompson = &thom;

    if(is_not_null(fp = fopen(file, "r"))) {

        thompson->nfa = nfa_alloc();
        thompson->top_state = -1;

        parse_tokens(
            grammar,
            scanner,
            (PScannerFunction *) &R_get_token,
            (void *) thompson,
            TREE_TRAVERSE_POSTORDER
        );

        printf("\nFreeing memory.. \n");

        grammar_free(grammar);

        nfa_print_dot(thompson->nfa);

        nfa_free(thompson->nfa);

        fclose(fp);
    } else {
        printf("Error: Could not open file '%s'.", file);
    }
}

/*
char terminal_names[19][40] = {
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
    "tab",
    "any_char"
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
*/

