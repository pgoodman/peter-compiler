/*
 * p-parser.c
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-parser.h>

#define P_SIZE_OF_THUNK (0 \
         + sizeof(PParserFunc) \
         + sizeof(PGenericList *) \
        ) / sizeof(char)

#define P_PRODUCTION_FAILED ((void *) 10)
#define P_SIZE_OF_REWRITE_RULE (sizeof(PParserRewriteRule) / sizeof(char))
#define P_SIZE_OF_PARSER_FUNC (sizeof(PParserFunc) / sizeof(char))

/* Type representing the necessary information to uniquely identify the result
 * of a production after being applied to a particular suffix of tokens. In this
 * case, list is some pointer to a generic list of tokens that the production was
 * applied to. The token list is ordered and never changes order and so the
 * result of a production to some part in the token list can be meaningfully
 * cached.
 */
typedef struct P_Thunk {
    PParserFunc production;
    PGenericList *list;
} P_Thunk;

/**
 * Type representing the cached result of the application of a production to
 * some part in the token list 'start'. This type is only used on successful
 * parse, the P_PRODUCTION_FAILED pointer is instead cached to indicate a failed
 * application of a production to some part of the token list.
 *
 * 'next' points to somewhere in the token list for the parser to pick up after
 * accepting the cached parse tree, 'tree', from the application of this production.
 *
 * 'start' and 'production' indicate what production was used and to what part
 * of the token list. This information is required in the event of resizing the
 * thunk hash table.
 */
typedef struct P_CachedResult {
    PGenericList *start,
                 *end;
    PParserFunc production;
    PParseTree *tree;
} P_CachedResult;

/**
 * Type representing a single production and all of its rules 'alternatives'
 * from a top-down parsing grammar. The list is of rules is explicitly ordered.
 *
 * The 'max_rule_elms' is maximum number of non/terminals in all of its rules.
 * This is used to allocate one and only one parse tree with max_rule_elms
 * branch pointers.
 */
typedef struct P_Production {
    PGenericList *alternatives;
    PParserFunc production;
    short max_rule_elms;
} P_Production;

/**
 * Type holding a pointer to the ordered list of *all* tokens from whatever text
 * is currently being parsed, as well as the number of tokens in that list.
 */
typedef struct P_TokenList {
    PGenericList *list;
    uint32_t num_tokens;
} P_TokenList;

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
 *    e) a do_backtrack flag indicating that a non/terminal in curr_rule_list
 *       has failed to match a token and so we must backtrack to the
 *       backtrack_point and try another rule, or upon full failure of the
 *       production, cascade the failure to the calling production.
 *    f) the parse tree as it is being constructed.
 *    g) the calling stack frame. This is used for two reasons, it lets us
 *       manually manage the call stack without the overhead of the generic
 *       stack data structure and it also lets us reclaim stack frames for
 *       future use.
 */
typedef struct P_StackFrame {
    P_Production *production;
    PGenericList *curr_rule_list,
                 *alternative_rules,
                 *backtrack_point;
    char do_backtrack;
    PParseTree *parse_tree;
    struct P_StackFrame *caller;
} P_StackFrame;

/**
 * Hash function that converts a thunk to a char array. Thunks are used as the
 * keys into a hash table for cached parse results.
 */
static uint32_t P_key_hash_thunk_fnc(void *pointer) {
    union {
        P_Thunk thunk;
        char thunk_as_chars[P_SIZE_OF_THUNK];
    } switcher;
    switcher.thunk = *((P_Thunk *) pointer);
    return murmur_hash(switcher.thunk_as_chars, P_SIZE_OF_THUNK, 73);
}

/**
 * Check for thunk collisions.
 */
static char P_hash_thunk_collision_fnc(void *thunk1, void *thunk2) {
    P_Thunk *t1 = thunk1,
            *t2 = thunk2;
    return ((t1->list != t2->list) || (t1->production != t2->production));
}

/**
 * Hash function that converts a rewrite rule (parser-func, lexeme) into a char
 * array to be hashed. Rewrite rules are hashable to allow us to make sure we
 * don't allocate duplicate rewrile rules where only one is really necessary.
 */
static uint32_t P_hash_rewrite_rule_fnc(void *rewrite_rule) {
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
static char P_hash_rewrite_collision_fnc(void *rule1, void *rule2) {
    PParserRewriteRule *r1 = rule1,
                       *r2 = rule2;
    return ((r1->func != r2->func) || (r1->lexeme != r2->lexeme));
}

/**
 * Hash function that converts a PParserFunc pointer into a char array. Hashing
 * parser functions lets us index the various productions in our top-down parsing
 * grammar. The parser production is used to index a P_Production.
 */
static uint32_t P_key_hash_production_fnc(void *production) {
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
static char P_hash_production_collision_fnc(void *fnc1, void *fnc2) {
    return fnc1 != fnc2;
}

/**
 * Allocate a new parser on the heap. A parser, in this case, is a container
 * linking to all of top-down-parsing language data structures as well as to
 * helping deal with garbage.
 *
 * The only parameter to this function is thee production to start parsing
 * with.
 */
PParser *parser_alloc(PParserFunc start_production) {
    PParser *P = mem_alloc(sizeof(PParser));
    if(is_null(P)) {
        mem_error("Unable to allocate a new parser on the heap.");
    }

    /* hash table mapping production function (PParserFunc) to the information
     * that corresponds with them (P_Production). */
    P->productions = dict_alloc(
        10,
        &P_key_hash_production_fnc,
        &P_hash_production_collision_fnc
    );

    /* hash table mapping parser rewrite rules to themselves. This is used
     * in order to not repeatedly heap allocate the same rule twice. A rewrite
     * rule identifies a token to match, a production to call, or nothing
     * (epsilon rule) that is a blind accept.
     */
    P->rules = dict_alloc(
        15,
        &P_hash_rewrite_rule_fnc,
        &P_hash_rewrite_collision_fnc
    );

    /* the cache table, this will be allocated when we know how many tokens we
     * are working with.
     */
    P->thunk_table = NULL;

    /* a place to put unused/temporary/garbage parse trees. */
    P->temp_parse_trees = stack_alloc(sizeof(PStack));

    /* signal that this parser is open to have productions and their rules added
     * to it.
     */
    P->is_closed = 0;

    /* the production to start parsing with. */
    P->start_production = start_production;

    return P;
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
    PGenericList *S = NULL,
                 *curr = NULL,
                 *tail = NULL;
    va_list seqs;
    assert_not_null(P);
    assert_not_null(semantic_handler_fnc);
    assert(0 < num_seqs);
    assert(!P->is_closed);
    assert(!dict_is_set(P->productions, semantic_handler_fnc));



    P_Production *prod = mem_alloc(sizeof(P_Production));
    if(is_null(prod)) {
        mem_error("Unable to allocate new production on the heap.");
    }

    prod->production = semantic_handler_fnc;
    prod->max_rule_elms = 0;

    va_start(seqs, arg1);
    for(curr_seq = arg1; \
        num_seqs > 0; \
        --num_seqs, curr_seq = va_arg(seqs, PParserRuleResult)) {

        if(prod->max_rule_elms < curr_seq.num_elms) {
            prod->max_rule_elms = curr_seq.num_elms;
        }

        curr = gen_list_alloc();
        gen_list_set_elm(curr, curr_seq.rule);

        /* add this into the sequence */
        if(is_not_null(tail)) {
            list_set_next(tail, curr);

        /* no tail ==> need to set the head of the list. */
        } else {
            S = curr;
        }

        tail = curr;
    }

    prod->alternatives = S;

    /* add in this production */
    dict_set(P->productions, semantic_handler_fnc, prod, &delegate_do_nothing);

    return;
}

/**
 * Create one of the rule sequences needed for a production. Each production
 * must have one or more rule sequence, where each rule sequence is a list of
 * PParserRewriteRule telling the parser to either match a particular token or
 * to recursively call and match a production.
 */
PParserRuleResult parser_rule_sequence(short num_rules, PParserRewriteRule *arg1, ...) {
    PParserRuleResult result;
    PParserRewriteRule *curr_rule = NULL;
    PGenericList *S = NULL,
                 *curr = NULL,
                 *tail = NULL;
    va_list rules;

    assert(0 < num_rules);
    assert_not_null(arg1);

    result.num_elms = num_rules;

    va_start(rules, arg1);
    for(curr_rule = arg1; \
        num_rules > 0; \
        --num_rules, curr_rule = va_arg(rules, PParserRewriteRule *)) {

        curr = gen_list_alloc();
        gen_list_set_elm(curr, curr_rule);

        /* add this into the sequence */
        if(is_not_null(tail)) {
            list_set_next(tail, curr);

        /* no tail ==> need to set the head of the list. */
        } else {
            S = curr;
        }

        tail = curr;
    }

    result.rule = S;
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
                                                 PLexeme tok,
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

    dict_set(P->rules, &rewrite_rule, R, &delegate_do_nothing);

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
PParserRewriteRule *parser_rewrite_token(PParser *P, PLexeme tok) {
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

/**
 * Allocate a new cached result of parsing on the heap.
 */
static P_CachedResult *P_alloc_cache(PGenericList *start,
                                     PGenericList *end,
                                     PParserFunc production,
                                     PParseTree *tree) {

    P_CachedResult *R = mem_alloc(sizeof(P_CachedResult));
    if(is_null(R)) {
        mem_error("Unable to cache the result of a production on the heap.");
    }

    R->start = start;
    R->end = end;
    R->production = production;
    R->tree = tree;

    return R;
}

/**
 * Allocate a new production stack frame on the heap or re-use a previously
 * allocated but now unused one.
 */
static P_StackFrame *P_frame_stack_alloc(P_StackFrame **unused,
                                         P_Production *prod,
                                         PGenericList *backtrack_point) {
    P_StackFrame *frame = NULL;

    if(is_null(*unused)) {
        frame = mem_alloc(sizeof(P_StackFrame));
        if(is_null(frame)) {
            mem_error("Unable to allocate new parser stack frame on the heap.");
        }
    } else {
        frame = (*unused);
        (*unused) = frame->caller;
    }

    frame->caller = NULL;
    frame->production = prod;
    frame->curr_rule_list = gen_list_get_elm(prod->alternatives);
    frame->alternative_rules = (PGenericList *) list_get_next(prod->alternatives);
    frame->backtrack_point = backtrack_point;
    frame->do_backtrack = 0;

    /* make the parse tree */
    frame->parse_tree = tree_alloc(
        sizeof(PProductionTree),
        (unsigned short) prod->max_rule_elms
    );

    frame->parse_tree->type = P_PARSE_TREE_PRODUCTION;
    ((PProductionTree *) (frame->parse_tree))->rule = 1;
    ((PProductionTree *) (frame->parse_tree))->production = prod->production;

    return frame;
}

/**
 * Push a stack frame onto the frame stack.
 */
static void P_frame_stack_push(P_StackFrame **stack, P_StackFrame *frame) {
    frame->caller = (*stack);
    (*stack) = frame;
    return;
}

/**
 * Pop a stack frame off of the frame stack and put it into the unused stack
 * where it will still be accessible.
 */
static void P_frame_stack_pop(P_StackFrame **unused, P_StackFrame **stack) {
    P_StackFrame *frame = (*stack);

    (*stack) = frame->caller;
    frame->caller = (*unused);
    (*unused) = frame;

    frame->alternative_rules = NULL;
    frame->backtrack_point = NULL;
    frame->caller = NULL;
    frame->curr_rule_list = NULL;
    frame->do_backtrack = 0;
    frame->parse_tree = NULL;
    frame->production = NULL;

    return;
}

/**
 * Return a linked list of all tokens in the current file being parsed.
 */
static P_TokenList P_get_all_tokens(PTokenGenerator *G) {

    P_TokenList list;
    PGenericList *prev = NULL,
                 *curr = NULL;

    assert_not_null(G);

    list.list = NULL;
    list.num_tokens = 0;

    /* build up a list of all of the tokens. */
    if(!generator_next(G)) {
        return list;
    }

    /* generate the first token */
    list.list = gen_list_alloc();
    gen_list_set_elm(list.list, generator_current(G));
    prev = list.list;
    list.num_tokens = 1;

    /* generate the rest of the tokens */
    while(generator_next(G)) {
        ++(list.num_tokens);
        curr = gen_list_alloc();
        gen_list_set_elm(curr, generator_current(G));
        list_set_next(prev, curr);
        prev = curr;
    }

    return list;
}

/**
 * Allocate a new thunk on the heap.
 */
static P_Thunk *P_alloc_thunk(PParserFunc func, PGenericList *list) {
    P_Thunk *thunk = mem_alloc(sizeof(P_Thunk));

    if(is_null(thunk)) {
        mem_error("Unable to heap allocate a thunk.");
    }

    thunk->list = list;
    thunk->production = func;

    return thunk;
}

/**
 * Parse the tokens from a token generator. This parser is an attempt at
 * implementing something similar to a packrat parser in C.
 *
 *       http://pdos.csail.mit.edu/~baford/packrat/thesis/thesis.pdf
 *
 * The parser starts by accumulating all of the tokens from the source text
 * into a large linked list.
 *
 * The parser then gets things started by pushing on a new stack frame
 * representing the application of the parser's starting production to the
 * the entire list of tokens, starting from the first token.
 *
 * At all times we maintain a pointer to where we are in the list of tokens.
 * This position can shift dramatically in the event that we don't match a
 * non/terminal and hence have to backtrack, or if we've previously matched
 * a production and hence have to jump ahead because we don't need to repeat
 * work that we've already done.
 *
 * All results from productions are cached. That is, if a production fails then
 * we cache that failure in the thunk table as a special pointer. If a production
 * succeeds then we cache its parse treee, where in the token list it started
 * and ended, and the production function itself.
 */
PParseTree *parser_parse_tokens(PParser *P, PTokenGenerator *G) {
    PToken *curr_token = NULL;
    P_Production *curr_production = NULL;
    P_TokenList token_result;
    PParseTree *parse_tree = NULL;
    PParserRewriteRule *curr_rule;
    P_StackFrame *frame = NULL,
                 *unused = NULL;
    PGenericList *curr = NULL;
    PTerminalTree *terminal_tree = NULL;
    P_CachedResult *cached_result = NULL;
    P_Thunk thunk;
    int num_comparisons = 0;

    assert_not_null(P);
    assert_not_null(G);
    assert(dict_is_set(P->productions, P->start_production));

    token_result = P_get_all_tokens(G);

    /* close the parser to updates on its grammar. */
    P->is_closed = 1;

    /* no tokens were lexed,
     * TODO: do something slightly more useful. */
    if(0 == token_result.num_tokens) {
        return parse_tree;
    }

    curr = token_result.list;

    /* allocate enough space for a single perfect pass through the text for
     * thunks.
     */
    P->thunk_table = dict_alloc(
        token_result.num_tokens,
        &P_key_hash_thunk_fnc,
        &P_hash_thunk_collision_fnc
    );

    /* get the starting production and push our first stack frame on. This
     * involves registering the start of the token list as the furthest back
     * we can backtrack.
     */
    curr_production = dict_get(P->productions, P->start_production);
    P_frame_stack_push(
        &frame,
        P_frame_stack_alloc(&unused, curr_production, curr)
    );

    parse_tree = frame->parse_tree;

    while(is_not_null(frame)) {

        /* get the rewrite rule and the current token we are looking at. */
        curr_production = frame->production;

        /* an error occurred in the current frame. we need to pop it off, dump
         * its parse tree, backtrack, and possibly cascade the failure upward.
         */
        if(frame->do_backtrack) {

            /* trim the (partial) branches off of the tree and put them in our
             * temporary stack. parts of these partial trees might represent
             * successful derivations and as such be part of the final parse
             * tree.
             */
            tree_trim(frame->parse_tree, P->temp_parse_trees);

            /* there are no rules to backtrack to, we need to cascade the
             * failure upward, dump the tree, and cache the failure.
             */
            if(is_null(frame->alternative_rules)) {

                printf("cascading.\n");

                /* dump the parse tree */
                tree_free(frame->parse_tree, &delegate_do_nothing);
                frame->parse_tree = NULL;

                /* cache the failure */
                dict_set(
                   P->thunk_table,
                   P_alloc_thunk(
                       curr_production->production,
                       frame->backtrack_point
                   ),
                   P_PRODUCTION_FAILED,
                   &delegate_do_nothing
                );

                /* pop the frame */
                P_frame_stack_pop(&unused, &frame);

                /* check if we encountered a parse error. */
                if(is_null(frame)) {
                    printf("parse error.\n");
                    exit(1);
                }

                /* the the frame on the top of the stack (callee) to backtrack. */
                frame->do_backtrack = 1;

            /* there is at least one rule to backtrack to. */
            } else {

                printf("backtracking.\n");

                /* switch rules */
                frame->curr_rule_list = (PGenericList *) gen_list_get_elm(
                    frame->alternative_rules
                );
                frame->alternative_rules = (PGenericList *) list_get_next(
                    frame->alternative_rules
                );

                /* backtrack to where this production tried to start matching
                 * from.
                 */
                curr = frame->backtrack_point;

                /* don't cascade */
                frame->do_backtrack = 0;
            }

        /* a production successfully matched all of the non/terminals in its
         * current rule list. we need to pop it off, merge its parse tree into
         * the parent frame's (calling production) parse tree, and then tell
         * the parent frame to advance itself to the next non/terminal in its
         * current rule list. if there is no parent frame then we have
         * successfully parsed the tokens.
         */
        } else if(is_null(frame->curr_rule_list)) {

            /* we have parsed all of the tokens. there is no need to do any
             * work on the stack or the cache. */
            if(is_null(frame->caller)) {

                printf("done parsing.\n");

                break;

            /* we can't prove that this is a successful parse of all of the
             * tokens and so we assume that there exists more to parse. save
             * the result of the application of this production to the cache
             * and yield control to the parent frame.
             */
            } else {

                printf("production succeeded.\n");

                /* cache the successful application of this production. */
                dict_set(
                    P->thunk_table,
                    P_alloc_thunk(
                        curr_production->production,
                        frame->backtrack_point
                    ),
                    P_alloc_cache(
                        frame->backtrack_point,
                        curr,
                        curr_production->production,
                        frame->parse_tree
                    ),
                    &delegate_do_nothing
                );

                /* merge the parse trees */
                tree_add_branch(frame->caller->parse_tree, frame->parse_tree);

                /* pop the frame */
                P_frame_stack_pop(&unused, &frame);

                /* tell the caller (now: frame) to advance its current rule list
                 * to the next rewrite rule.
                 */
                frame->curr_rule_list = (PGenericList *) list_get_next(
                    frame->curr_rule_list
                );
            }

        } else if(is_not_null(curr)) {

            curr_token = gen_list_get_elm(curr);
            curr_rule = gen_list_get_elm(frame->curr_rule_list);

            /* the next non/terminal in the current rule list is a non-terminal,
             * i.e. it is a production. we need to push the production onto the
             * stack.
             */
            if(is_not_null(curr_rule->func)) {

                /* check if we have a cached this result of applying this
                 * particularproduction to our current position in the token
                 * list. */
                thunk.production = curr_rule->func;
                thunk.list = curr;
                cached_result = dict_get(P->thunk_table, &thunk);

                /* we have a cached result. */
                if(is_not_null(cached_result)) {

                    /* the cached result is a failure. time to backtrack. */
                    if(cached_result == P_PRODUCTION_FAILED) {

                        printf("cached production failed.\n");

                        frame->do_backtrack = 1;

                    /* the cached result was a success, we need to merge the
                     * cached tree with our current parse tree, advance to the
                     * next rewrite rule in our current rule list, and advance
                     * our current position in the token list to the token after
                     * the last token that was matched in the cached result.
                     */
                    } else {

                        printf("cached production succeeded.\n");

                        /* merge the trees */
                        tree_add_branch(frame->parse_tree, cached_result->tree);

                        /* advance to the next rewrite rule */
                        frame->curr_rule_list = (PGenericList *) list_get_next(
                            frame->curr_rule_list
                        );

                        /* advance to a future token. */
                        curr = cached_result->end;
                    }

                /* we do not have a cached result and so we will need to push a
                 * new frame onto the stack.
                 */
                } else {

                    printf("pushing production onto stack.\n");

                    P_frame_stack_push(&frame, P_frame_stack_alloc(
                        &unused,
                        dict_get(P->productions, curr_rule->func),
                        curr
                    ));
                }

            /* the next non/terminal in the current rule list is a terminal,
             * i.e. it is a token. we need to try to match the current token's
             * (curr_token) lexeme against the rule's lexeme. if they match then
             * we need to advance to the next rule and to the next token. if
             * they do not match then we need to signal the frame to backtrack.
             */
            } else if(P_LEXEME_EPSILON != curr_rule->lexeme) {

                ++num_comparisons;

                /* we have matched a token, advance to the next token in the
                 * list and the next rewrite rule in the current rule list.
                 * also, store the matched token into the frame's partial parse
                 * tree. */
                if(curr_token->lexeme == curr_rule->lexeme) {

                    printf("matched: %s\n", curr_token->val->str);

                    /* advance the token and rewrite rule */
                    curr = (PGenericList *) list_get_next(curr);
                    frame->curr_rule_list = (PGenericList *) list_get_next(
                        frame->curr_rule_list
                    );

                    /* store the match as a parse tree */
                    terminal_tree = tree_alloc(sizeof(PTerminalTree), 0);
                    terminal_tree->token = curr_token;
                    ((PParseTree *) terminal_tree)->type = P_PARSE_TREE_TERMINAL;
                    tree_add_branch(frame->parse_tree, terminal_tree);

                /* the tokens do not match, backtrack. */
                } else {

                    printf("didn't match, expected:%d got:%d\n", curr_rule->lexeme, curr_token->lexeme);

                    frame->do_backtrack = 1;
                }

            /* the next non/termrnal in the current rule list is an empty
             * terminal, i.e. it is the empty string. we accept it, move to the
             * next rule in the current rule list, but *don't* move to the next
             * token. */
            } else {
                frame->curr_rule_list = (PGenericList *) list_get_next(
                    frame->curr_rule_list
                );
            }

        /* force an upward cascade of successful operations when none of the
         * other conditions are met. This ensures that we finish by putting
         * together the entire parse tree. */
        } else {
            frame->curr_rule_list = (PGenericList *) list_get_next(
                frame->curr_rule_list
            );
        }
    }

    printf("completed parse with %d token comparisons.\n", num_comparisons);
    return parse_tree;
}

