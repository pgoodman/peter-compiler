/*
 * adt.c
 *
 *  Created on: May 19, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-adt.h>

#define P_SIZE_OF_REWRITE_RULE (sizeof(PParserRewriteRule) / sizeof(char))
#define P_SIZE_OF_PARSER_FUNC (sizeof(PParserFunc) / sizeof(char))


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
    return ((r1->func != r2->func) || (r1->lexeme != r2->lexeme));
}

/**
 * Hash function that converts a PParserFunc pointer into a char array. Hashing
 * parser functions lets us index the various productions in our top-down parsing
 * grammar. The parser production is used to index a P_Production.
 */
static uint32_t P_production_hash_fnc(PParserFunc production) {
    union {
        PParserFunc prod;
        char prod_as_chars[P_SIZE_OF_PARSER_FUNC];
    } switcher;

    switcher.prod = (PParserFunc) production;
    return murmur_hash(switcher.prod_as_chars, P_SIZE_OF_PARSER_FUNC, 73);
}

/**
 * Check for a hash collision.
 */
static char P_production_collision_fnc(PParserFunc fnc1, PParserFunc fnc2) {
    return fnc1 != fnc2;
}


/**
 * Allocate a new parser on the heap. A parser, in this case, is a container
 * linking to all of top-down-parsing language data structures as well as to
 * helping deal with garbage.
 */
PParser *parser_alloc(PParserFunc start_production,
                      size_t num_tokens,
                      size_t num_useful_tokens,
                      short useful_tokens[]) {

    PParser *P = mem_alloc(sizeof(PParser));
    size_t i;

    if(is_null(P)) {
        mem_error("Unable to allocate a new parser on the heap.");
    }

    /* hash table mapping production function (PParserFunc) to the information
     * that corresponds with them (P_Production). */
    P->productions = prod_dict_alloc(
        10,
        &P_production_hash_fnc,
        &P_production_collision_fnc
    );

    /* hash table mapping parser rewrite rules to themselves. This is used
     * in order to not repeatedly heap allocate the same rule twice. A rewrite
     * rule identifies a token to match, a production to call, or nothing
     * (epsilon rule) that is a blind accept.
     */
    P->rules = dict_alloc(
        15,
        &P_rewrite_rule_hash_fnc,
        &P_rewrite_rule_collision_fnc
    );

    /* table for tokens and their usefulness. */
    P->num_tokens = num_tokens;
    P->token_is_useful = mem_calloc(num_tokens, sizeof(short));
    if(is_null(P->token_is_useful)) {
        mem_error("Unable to allocate token useefulness table on the heap.");
    }
    for(i = 0; i < num_useful_tokens; ++i) {
        P->token_is_useful[useful_tokens[i]] = 1;
    }

    /* signal that this parser is open to have productions and their rules added
     * to it.
     */
    P->is_closed = 0;

    /* the production to start parsing with. */
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
static void P_free_production_val(PParserProduction *P) {
    assert_not_null(P);
    gen_list_free_chain(P->alternatives, (PDelegate) &P_free_alternative_rules);
    mem_free(P);
}

static void P_free_production_key(PParserFunc F) {
    /* do nothing */
}

static void P_free_production_val_ignore(PParserProduction *P) {
    /* do nothing */
}

/**
 * Free the parser adt.
 */
void parser_free(PParser *P) {
    assert_not_null(P);
    prod_dict_free(
        P->productions,
        &P_free_production_val,
        &P_free_production_key
    );
    dict_free(P->rules, &delegate_mem_free, &delegate_do_nothing);
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
                           PParserFunc semantic_handler_fnc, /* the production name */
                           short num_seqs, /* number of rewrite sequences */
                           PParserRuleResult arg1, ...) { /* rewrite rules */
    PParserRuleResult curr_seq;
    PParserProduction *prod = NULL;
    PGenericList *curr = NULL;

    va_list seqs;

    assert_not_null(P);
    assert_not_null(semantic_handler_fnc);
    assert(0 < num_seqs);
    assert(!P->is_closed);
    assert(!prod_dict_is_set(P->productions, semantic_handler_fnc));

    prod = mem_alloc(sizeof(PParserProduction));
    if(is_null(prod)) {
        mem_error("Unable to allocate new production on the heap.");
    }

    prod->production = semantic_handler_fnc;
    prod->max_num_useful_rewrite_rules = 0;
    prod->alternatives = gen_list_alloc_chain(num_seqs);

    va_start(seqs, arg1);

    curr_seq = arg1;
    curr = prod->alternatives;

    for(; is_not_null(curr); curr = (PGenericList *) list_get_next(curr)) {

        if(prod->max_num_useful_rewrite_rules < curr_seq.num_useful_elms) {
            prod->max_num_useful_rewrite_rules = curr_seq.num_useful_elms;
        }

        gen_list_set_elm(curr, curr_seq.rule);
        curr_seq = va_arg(seqs, PParserRuleResult);
    }

    /* add in this production */
    prod_dict_set(
        P->productions,
        semantic_handler_fnc,
        prod,
        &P_free_production_val_ignore
    );

    return;
}

/**
 * Create one of the rule sequences needed for a production. Each production
 * must have one or more rule sequence, where each rule sequence is a list of
 * PParserRewriteRule telling the parser to either match a particular token or
 * to recursively call and match a production.
 */
PParserRuleResult parser_rule_sequence(PParser *P, short num_rules, PParserRewriteRule *arg1, ...) {
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

    for(; is_not_null(curr); curr = (PGenericList *) list_get_next(curr)) {
        gen_list_set_elm(curr, curr_rule);

        /* this is not a useful rule, make sure that we know not to record
         * it in any parse trees. */
        if(is_null(curr_rule->func)
           && (curr_rule->lexeme == P_LEXEME_EPSILON
               || !(P->token_is_useful[(int) curr_rule->lexeme]))) {

            --(result.num_useful_elms);
        }

        curr_rule = va_arg(rules, PParserRewriteRule *);
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
                                                 char tok,
                                                 PParserFunc func) {
    PParserRewriteRule rewrite_rule;
    PParserRewriteRule *R = NULL;

    assert_not_null(P);

    /* make a thunk out of it to search for in the hash table */
    rewrite_rule.func = func;
    rewrite_rule.lexeme = tok;

    if(dict_is_set(P->rules, &rewrite_rule)) {
        return ((PParserRewriteRule *) dict_get(P->rules, &rewrite_rule));
    }

    /* nope, need to allocate it :( */
    R = mem_alloc(sizeof(PParserRewriteRule));
    if(is_null(R)) {
        mem_error("Unable to allocate rewrite rule on the heap.");
    }

    R->func = func;
    R->lexeme = tok;

    dict_set(P->rules, R, R, &delegate_do_nothing);

    return R;
}

/**
 * Rewrite rule for a single production function
 */
PParserRewriteRule *parser_rewrite_function(PParser *P, PParserFunc func) {
    assert_not_null(P);
    assert_not_null(func);
    return P_parser_rewrite_rule(P, P_LEXEME_EPSILON, func);
}

/**
 * Rewrite rule for a single token
 */
PParserRewriteRule *parser_rewrite_token(PParser *P, char tok) {
    assert_not_null(P);
    return P_parser_rewrite_rule(P, tok, NULL);
}

/**
 * Rewrite rule for no token required.
 */
PParserRewriteRule *parser_rewrite_epsilon(PParser *P) {
    assert_not_null(P);
    return P_parser_rewrite_rule(P, P_LEXEME_EPSILON, NULL);
}
