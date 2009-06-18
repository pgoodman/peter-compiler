/*
 * adt.c
 *
 *  Created on: May 19, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-grammar.h>
#include <p-grammar-internal.h>

#define C_PRODUCTION_RULES 0
#define C_PRODUCTION_RULE_PHRASES 1
#define C_PHRASES 2
#define C_SYMBOLS 3

#define P_SYMBOL_IS_NON_TERMINAL 1
#define P_SYMBOL_IS_TERMINAL 2
#define P_SYMBOL_IS_EPSILON 4
#define P_SYMBOL_IS_NON_EXCLUDABLE 8
#define P_SYMBOL_RAISE_CHILDREN 16

/* -------------------------------------------------------------------------- */

static G_Symbol *G_get_next_symbol(PGrammar *grammar) {

    assert_not_null(grammar);
    assert(!grammar->is_locked);
    assert(grammar->counter[C_SYMBOLS] < grammar->num_symbols);

    return grammar->symbols + ((grammar->counter[C_SYMBOLS])++);
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a new parser on the heap. A parser, in this case, is a container
 * linking to all of top-down-parsing language data structures as well as to
 * helping deal with garbage.
 */
PGrammar *grammar_alloc(G_NonTerminal start_production,
                        short num_productions,
                        short num_tokens,
                        short num_phrases,
                        short num_symbols) {

    PGrammar *grammar;
    char *data;

    data = mem_alloc(
        sizeof(PParser) + \
        (num_productions * sizeof(G_ProductionRule)) + \
        (num_phrases * sizeof(G_Phrase)) + \
        (num_symbols * sizeof(G_Symbol))
    );

    if(is_null(data)) {
        mem_error("Unable to allocate a new parser on the heap.");
    }

    grammar = (PGrammar *) data;
    grammar->num_phrases = num_phrases;
    grammar->num_productions = num_productions;
    grammar->num_symbols = num_symbols;
    grammar->num_tokens = num_tokens;

    grammar->is_locked = 0;
    grammar->start_production_rule = start_production;

    grammar->counter[C_PRODUCTION_RULES] = 0;
    grammar->counter[C_PRODUCTION_RULE_PHRASES] = 0;
    grammar->counter[C_PHRASES] = 0;
    grammar->counter[C_SYMBOLS] = 0;

    grammar->production_rules = (G_ProductionRule *) (
        data + (sizeof(PGrammar) / sizeof(char))
    );

    grammar->phrases = (G_Phrase *) (
        data + (
            (sizeof(PGrammar) +
            (2 * num_productions * sizeof(G_ProductionRule)))
            / sizeof(char)
        )
    );

    grammar->symbols = (G_Symbol *) (
        data + (
            (sizeof(PGrammar) +
            (2 * num_productions * sizeof(G_ProductionRule)) +
            (num_phrases * sizeof(G_Phrase)))
            / sizeof(char)
        )
    );

    return grammar;
}
/**
 * Free the parser adt.
 */
void grammar_free(PGrammar *grammar) {
    assert_not_null(grammar);
    mem_free(grammar);
}

/**
 * Add a production rule to the grammar. In the following grammar:
 *
 *     A --> aB C
 *       --> dE F
 *
 *     D --> x
 *
 * "A" and "D" are productions and "A --> ...; --> ..." is a production rule.
 * The production rule for the production "A" has two phrases in it. The
 * production rule for the production "D" has only one phrase.
 */
void grammar_add_production_rule(PGrammar *grammar, G_NonTerminal production) {

    G_ProductionRule *rule,
                     *prev_rule;

    unsigned int which_rule;

    assert_not_null(grammar);
    assert(!grammar->is_locked);
    assert(production < grammar->num_productions);

    which_rule = grammar->counter[C_PRODUCTION_RULES];

    printf("adding rule %d as %d \n", production, which_rule);

    assert(which_rule < grammar->num_productions);

    rule = grammar->production_rules + ((unsigned int) production);

    if(0 == which_rule) {
        rule->num_phrases = grammar->counter[C_PHRASES];
        rule->phrases = grammar->phrases;
    } else {
        rule->phrases = grammar->phrases
                      + grammar->counter[C_PRODUCTION_RULE_PHRASES];

        rule->num_phrases = grammar->counter[C_PHRASES]
                          - grammar->counter[C_PRODUCTION_RULE_PHRASES];
    }

    grammar->counter[C_PRODUCTION_RULE_PHRASES] = grammar->counter[C_PHRASES];

    ++(grammar->counter[C_PRODUCTION_RULES]);

    rule->production = production;
    return;
}

/**
 * Add a phrase to the grammar. In the following grammar:
 *
 *     A --> aB C
 *       --> dE F
 *
 * Both of "aB C" and "dE F" are phrases.
 */
void grammar_add_phrase(PGrammar *grammar) {

    G_Phrase *phrase,
             *prev_phrase;

    unsigned int which_phrase;

    assert_not_null(grammar);
    assert(!grammar->is_locked);

    which_phrase = grammar->counter[C_PHRASES];

    assert(which_phrase < grammar->num_phrases);

    phrase = grammar->phrases + which_phrase;

    /* this is the first phrase being added */
    if(0 == which_phrase) {
        phrase->num_symbols = grammar->counter[C_SYMBOLS];
        phrase->symbols = grammar->symbols;

    /* this is the nth phrase being added. */
    } else {
        prev_phrase = grammar->phrases + (which_phrase - 1);
        phrase->symbols = prev_phrase->symbols + prev_phrase->num_symbols;
        phrase->num_symbols = (
            (grammar->symbols + grammar->counter[C_SYMBOLS]) - phrase->symbols
        );
    }

    ++(grammar->counter[C_PHRASES]);
}

/**
 * Rewrite rule for a single production.
 */
void grammar_add_non_terminal_symbol(PGrammar *grammar,
                                     G_NonTerminal production,
                                     unsigned char is_non_excludable,
                                     unsigned char must_be_raised) {
    G_Symbol *symbol = G_get_next_symbol(grammar);
    symbol->flag = (
        P_SYMBOL_IS_NON_TERMINAL \
        | (is_non_excludable ? P_SYMBOL_IS_NON_EXCLUDABLE : 0)
        | (must_be_raised ? P_SYMBOL_RAISE_CHILDREN : 0)
    );
    symbol->value.non_terminal = production;
}

/**
 * Rewrite rule for a single token.
 */
void grammar_add_terminal_symbol(PGrammar *grammar,
                                 G_Terminal token,
                                 unsigned char is_non_excludable) {
    G_Symbol *symbol = G_get_next_symbol(grammar);
    symbol->flag = (
        P_SYMBOL_IS_TERMINAL \
        | (is_non_excludable ? P_SYMBOL_IS_NON_EXCLUDABLE : 0)
    );
    symbol->value.terminal = token;
}

/**
 * Rewrite rule for no token required.
 */
void grammar_add_epsilon_symbol(PGrammar *grammar,
                                unsigned char is_non_excludable) {
    G_get_next_symbol(grammar)->flag = (
        P_SYMBOL_IS_EPSILON \
        | (is_non_excludable ? P_SYMBOL_IS_NON_EXCLUDABLE : 0)
    );
}

/* -------------------------------------------------------------------------- */

/**
 * Lock a grammar from further adding of stuff.
 */
void G_lock(PGrammar *grammar) {
    assert_not_null(grammar);
    grammar->is_locked = 1;
}

/**
 * Check if a given production rule has a phrase.
 */
int G_production_rule_has_phrase(G_ProductionRule *rule, unsigned int phrase) {
    assert_not_null(rule);
    return phrase < rule->num_phrases;
}

/**
 * Get a specific symbol from a specific phrase in a production rule. If that
 * symbol does not exist then return null.
 */
G_Symbol *G_production_rule_get_symbol(G_ProductionRule *rule,
                                       unsigned int which_phrase,
                                       unsigned int which_symbol) {

    G_Phrase *phrase;

    assert_not_null(rule);
    assert(which_phrase < rule->num_phrases);

    phrase = rule->phrases + which_phrase;

    /* we've got all of the symbols from this particular phrase, i.e. we've
     * successfully matched all of the symbols in a phrase. */
    if(which_symbol >= phrase->num_symbols) {
        return NULL;
    }

    return phrase->symbols + which_symbol;
}

/**
 * Return whether or not the current symbol cannot be exluded from the parse tree.
 */
int G_symbol_is_non_excludable(G_Symbol *symbol) {
    assert_not_null(symbol);
    return symbol->flag & P_SYMBOL_IS_NON_EXCLUDABLE;
}

/**
 * Return whether or not the children nodes in the parse tree should be
 * promoted.
 */
int G_symbol_use_children_instead(G_Symbol *symbol) {
    assert_not_null(symbol);
    return symbol->flag & P_SYMBOL_RAISE_CHILDREN;
}

/**
 * Return whether or not a rule is a production rule.
 */
int G_symbol_is_non_terminal(G_Symbol *symbol) {
    assert_not_null(symbol);
    return symbol->flag & P_SYMBOL_IS_NON_TERMINAL;
}

/**
 * Return whether or not a rule is a token rule.
 */
int G_symbol_is_terminal(G_Symbol *symbol) {
    assert_not_null(symbol);
    return symbol->flag & P_SYMBOL_IS_TERMINAL;
}

/**
 * Return whether or not a rule is an epsilon rule.
 */
int G_symbol_is_epsilon(G_Symbol *symbol) {
    assert_not_null(symbol);
    return symbol->flag & P_SYMBOL_IS_EPSILON;
}