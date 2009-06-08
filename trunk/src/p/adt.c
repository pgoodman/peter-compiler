/*
 * adt.c
 *
 *  Created on: May 19, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-adt.h>

#define P_TOKEN_EPSILON ((unsigned char) 0xff)
#define P_SIZE_OF_REWRITE_RULE (sizeof(PParserRewriteRule) / sizeof(char))

/**
 * Hash function that converts a rewrite rule (parser-func, lexeme) into a char
 * array to be hashed. Rewrite rules are hashable to allow us to make sure we
 * don't allocate duplicate rewrile rules where only one is really necessary.
 */
static uint32_t P_rewrite_rule_hash_fnc(void *rewrite_rule) {

    union {
        PParserRewriteRule rewrite;
        char rule_as_chars[P_SIZE_OF_REWRITE_RULE];
    } switcher;

    switcher.rewrite = *((PParserRewriteRule *) rewrite_rule);
    return murmur_hash(switcher.rule_as_chars, P_SIZE_OF_REWRITE_RULE, 73);
}

/**
 * Check for a collision in a rewrite rule hash.
 */
static char P_rewrite_rule_collision_fnc(void *rule1, void *rule2) {
    PParserRewriteRule *r1 = rule1,
                       *r2 = rule2;
    return ((r1->production != r2->production) || (r1->token != r2->token));
}

/**
 * Allocate a new parser on the heap. A parser, in this case, is a container
 * linking to all of top-down-parsing language data structures as well as to
 * helping deal with garbage.
 */
PParser *parser_alloc(unsigned char start_production,
                      size_t num_productions,
                      size_t num_tokens) {

    PParser *P = mem_alloc(sizeof(PParser));
    size_t i;

    if(is_null(P)) {
        mem_error("Unable to allocate a new parser on the heap.");
    }

    /* hash table mapping production to the information that corresponds with
     * them (P_Production). */
    P->num_productions = num_productions;
    P->productions = mem_calloc(num_productions, sizeof(P_Production));
    if(is_null(P->productions)) {
        mem_error("Unable to allocate productions table on the heap.");
    }

    /* hash table mapping parser rewrite rules to themselves. This is used
     * in order to not repeatedly heap allocate the same rule twice. A rewrite
     * rule identifies a token to match, a production to call, or nothing
     * (epsilon rule) that is a blind accept.
     *
     * TODO: Use a proper Set adt here instead
     */
    P->rules = dict_alloc(
        15,
        &P_rewrite_rule_hash_fnc,
        &P_rewrite_rule_collision_fnc
    );

    P->num_tokens = num_tokens;
    P->is_closed = 0;
    P->start_production = start_production;

    return P;
}

/**
 * Free one of the lists of rewrite rules for a production.
 */
static void P_free_alternative_rules(PGenericList *L) {
    gen_list_free_chain(L, &delegate_do_nothing);
}

/**
 * Free a production and all of its alternative rewrite rules.
 */
static void P_free_production_val(P_Production *P) {
    assert_not_null(P);
    gen_list_free_chain(P->alternatives, (PDelegate) &P_free_alternative_rules);
    mem_free(P);
}

/**
 * Free the parser adt.
 */
void parser_free(PParser *P) {
    unsigned int i;
    P_Production prod;

    assert_not_null(P);

    for(i = 0; i < P->num_productions; ++i) {
        gen_list_free_chain(
            P->productions[i].alternatives,
            (PDelegate) &P_free_alternative_rules
        );
    }

    mem_free(P->productions);
    dict_free(P->rules, &delegate_do_nothing, &delegate_mem_free);
    mem_free(P);
}

/**
 * Add a production to the parser's grammar. The production's name is the name
 * (or rather the function pointer) of the parser function that handles semantic
 * actions on the parse tree. This function pointer is used to reference this
 * production in the rules.
 *
 * !!! Rules are *ordered*
 */
void parser_add_production(PParser *P,
                           unsigned char production,
                           short num_seqs,
                           PParserRuleResult arg1, ...) {

    PParserRuleResult curr_seq;
    P_Production *prod;
    PGenericList *curr = NULL;

    va_list seqs;

    assert_not_null(P);
    assert(production < P->num_productions);
    assert(0 < num_seqs);
    assert(!P->is_closed);

    prod = P->productions + production;

    prod->production = production;
    prod->max_num_useful_rewrite_rules = 0;
    prod->alternatives = gen_list_alloc_chain(num_seqs);

    va_start(seqs, arg1);

    curr_seq = arg1;
    curr = prod->alternatives;

    for(; is_not_null(curr); ) {
        if(prod->max_num_useful_rewrite_rules < curr_seq.num_useful_elms) {
            prod->max_num_useful_rewrite_rules = curr_seq.num_useful_elms;
        }

        gen_list_set_elm(curr, curr_seq.rule);
        curr_seq = va_arg(seqs, PParserRuleResult);
        curr = (PGenericList *) list_get_next((PList *) curr);
    }

    return;
}

/**
 * Create one of the rule sequences needed for a production. Each production
 * must have one or more rule sequence, where each rule sequence is a list of
 * PParserRewriteRule telling the parser to either match a particular token or
 * to recursively call and match a production.
 */
PParserRuleResult parser_rule_sequence(PParser *P,
                                       short num_rules,
                                       PParserRewriteRule *arg1, ...) {
    PParserRuleResult result;
    PParserRewriteRule *curr_rule = NULL;
    PGenericList *curr = NULL;
    va_list rules;

    assert_not_null(P);
    assert(0 < num_rules);
    assert_not_null(arg1);

    va_start(rules, arg1);

    result.num_useful_elms = num_rules;
    result.rule = gen_list_alloc_chain(num_rules);

    curr_rule = arg1;
    curr = result.rule;

    for(; is_not_null(curr); ) {
        gen_list_set_elm(curr, curr_rule);

        /* this is not a useful rule, make sure that we know not to record
         * it in any parse trees. */
        if(P_TOKEN_EPSILON == curr_rule->production
        && !curr_rule->flag) {
            --(result.num_useful_elms);
        }

        curr_rule = va_arg(rules, PParserRewriteRule *);
        curr = (PGenericList *) list_get_next((PList *) curr);
    }

    return result;
}

/**
 * A generic parser rewrite rule. This encompasses all three types of things that
 * we want to match:
 *   1) production (function) rules
 *   2) token rules
 *   3) epsilon rules.
 */
static PParserRewriteRule *P_parser_rewrite_rule(PParser *P,
                                                 const unsigned char tok,
                                                 const unsigned char prod,
                                                 unsigned char flag) {
    PParserRewriteRule s_rule,
                       *rule = NULL;

    assert_not_null(P);

    /* make a thunk out of it to search for in the hash table */
    s_rule.production = prod;
    s_rule.token = tok;
    s_rule.flag = flag;

    if(dict_is_set(P->rules, &s_rule)) {
        return ((PParserRewriteRule *) dict_get(P->rules, &s_rule));
    }

    /* nope, need to allocate it :( */
    rule = mem_alloc(sizeof(PParserRewriteRule));
    if(is_null(rule)) {
        mem_error("Unable to allocate rewrite rule on the heap.");
    }

    rule->production = prod;
    rule->token = tok;
    rule->flag = flag;

    dict_set(P->rules, rule, rule, &delegate_do_nothing);

    return rule;
}

/**
 * Rewrite rule for a single production.
 */
PParserRewriteRule *parser_rewrite_production(PParser *P,
                                              unsigned char prod,
                                              unsigned char flag) {
    assert_not_null(P);
    assert(prod < P->num_productions);
    return P_parser_rewrite_rule(P, P_TOKEN_EPSILON, prod, flag);
}

/**
 * Rewrite rule for a single token.
 */
PParserRewriteRule *parser_rewrite_token(PParser *P,
                                         unsigned char tok,
                                         unsigned char flag) {
    assert_not_null(P);
    assert(tok < P->num_tokens);
    return P_parser_rewrite_rule(P, tok, P_TOKEN_EPSILON, flag);
}

/**
 * Rewrite rule for no token required.
 */
PParserRewriteRule *parser_rewrite_epsilon(PParser *P,
                                           unsigned char flag) {
    assert_not_null(P);
    return P_parser_rewrite_rule(
        P,
        P_TOKEN_EPSILON,
        P_TOKEN_EPSILON,
        flag
    );
}

/**
 * Return whether or not a rule is a production rule.
 */
int P_adt_rule_is_production(PParserRewriteRule *rule) {
    assert_not_null(rule);
    return P_TOKEN_EPSILON != rule->production;
}

/**
 * Return whether or not a rule is a token rule.
 */
int P_adt_rule_is_token(PParserRewriteRule *rule) {
    assert_not_null(rule);
    return P_TOKEN_EPSILON != rule->token;
}

/**
 * Return whether or not a rule is an epsilon rule.
 */
int P_adt_rule_is_epsilon(PParserRewriteRule *rule) {
    assert_not_null(rule);
    return (P_TOKEN_EPSILON == rule->token)
        && (P_TOKEN_EPSILON == rule->production);
}

/**
 * Return whether or not the current rule can be exluded from the parse tree.
 */
int P_adt_rule_is_non_excludable(PParserRewriteRule *rule) {
    assert_not_null(rule);
    return (rule->flag == 1);
}

/**
 * Return whether or not the children nodes in the parse tree should be
 * promoted.
 */
int P_adt_rule_use_children_instead(PParserRewriteRule *rule) {
    assert_not_null(rule);
    return (rule->flag == 2);
}
