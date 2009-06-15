/*
 * p-parser-types.h
 *
 *  Created on: May 15, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PPARSERTYPES_H_
#define PPARSERTYPES_H_

#define P_MAX_RECURSION_DEPTH 50

#include "p-lexer.h"
#include "p-common-types.h"

/* base parse types, holds our rewrite rules. */
typedef struct PGrammar {

    /* on for when we no longer allow rules to be added. */
    unsigned char is_locked;

    /* keep track of all of the productions for the parsing grammar. */
    G_ProductionRule *production_rules;

    G_Phrase *phrases;

    G_Symbol *symbols;

    short num_productions,
          num_tokens,
          num_phrases,
          num_symbols;

    /* for counting where we are in building the parser */
    short counter[3];

    G_NonTerminal start_production_rule;

} PGrammar;

/* -------------------------------------------------------------------------- */

/**
 * Type representing the cached result of the application of a production to
 * some part in the token list 'start'. This type is only used on successful
 * parse, the P_PRODUCTION_FAILED pointer is instead cached to indicate a failed
 * application of a production to some part of the token list.
 *
 * 'end' points to somewhere in the token list for the parser to pick up after
 * accepting the cached parse tree, 'tree', from the application of this
 * production.
 */
typedef struct P_IntermediateResult {

    PT_Terminal *end_token;

    PParseTree *intermediate_tree;

    char is_left_recursive;

} P_IntermediateResult;

/* -------------------------------------------------------------------------- */

/**
 * In a sense this parser is recursive-descent; however, it maintains its own
 * heap-allocated stack frames, as opposed to calling production functions
 * recursively.
 *
 * The choice to manage stack frames explicitly has a few very nice benefits:
 *   i) it allows for the functions that perform the actual parsing to be
 *      isolated and for the operations of the parser to be clearly defined and
 *      predictable.
 *   ii) it allows for the actual parser to be defined entirely in terms of
 *       primitive pattern matching against the grammar as opposed to requiring
 *       independent functions to be constructed to perform the actual parsing.
 *   iii) it allows for a one-to-one correspondence between the definition of
 *        the parser in C and the top-down parsing language grammar.
 *
 * A stack frame holds important information, namely:
 *    a) the current production in being executed.
 *    b) a list of the remaining non/terminals to match against tokens in the
 *       token list. When this list is NULL it implies that we have reached the
 *       end of it and thus have matched all of the non/terminals of this
 *       particular production's rule.
 *    c) the alternative rules if any non/terminal in the curr_rule_list fails
 *       to match. If the alternative_rules list is null and the 'do_backtrack'
 *       flag is set then it implies that all of the rules for the current
 *       production have failed to match the tokens.
 *    d) the backtrack_point points to where in the token list the parser was
 *       when it attempted to start matching this production. This is the point
 *       where it will return to on failure in order to try matching another
 *       rule.
 *    e) a do_backtrack flag indicating that a non/terminal in
 *       curr_rule_list has failed to match a token and so we must
 *       backtrack to the backtrack_point and try another rule, or upon
 *       full failure of the production, cascade the failure to the
 *       calling production.
 *    f) the parse tree as it is being constructed.
 *    g) the calling stack frame. This is used for two reasons, it lets us
 *       manually manage the call stack without the overhead of the generic
 *       stack data structure and it also lets us reclaim stack frames for
 *       future use.
 */
typedef struct P_Frame {

    /* the current production rule being used by this call frame */
    struct {
        G_ProductionRule *rule;
        unsigned char phrase,
                      symbol;
    } production;

    /* the left recursive rule, if any */
    struct {
        G_Symbol *symbol;

        char is_direct,
             is_used;
    } left_recursion;

    /* deal with backtracking */
    PT_Terminal *backtrack_point;

    /* the farthest id into the token stream that we've reached. */
    uint32_t farthest_id_reached;

    PParseTree *parse_tree;

} P_Frame;

/* -------------------------------------------------------------------------- */

/* The current state of the parser. */
typedef struct PParser {

    uint32_t num_tokens;

    /* the cache table, this is a two dimensional array */
    P_IntermediateResult **intermediate_results;

    /* the parser call stack */
    struct {
        P_Frame *stack[P_MAX_RECURSION_DEPTH];
        char frame;
    } call;

    /* the set of all trees, ends up being the garbage set */
    PT_Set *trees;

    /* the farthest id into the token stream that we've reached. */
    uint32_t farthest_id_reached;

    /* whether or not a backtrack is required */
    char must_backtrack;

} PParser;

#endif /* PPARSERTYPES_H_ */
