/*
 * frame.c
 *
 *  Created on: Jun 2, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <p-production.h>

#define prod_mem_alloc(x) mem_alloc(x); ++num_allocations
#define prod_mem_calloc(x,y) mem_calloc(x,y); ++num_allocations
#define prod_mem_free(x) mem_free(x); --num_allocations
#define prod_mem_error(x) mem_error(x)
static unsigned long int num_allocations = 0;
unsigned long int prod_num_allocated_pointers(void) {
    return num_allocations;
}

/**
 * Allocate a production stack.
 */
P_ProductionStack *P_production_alloc(void) {
    P_ProductionStack *stack = prod_mem_alloc(sizeof(P_ProductionStack));
    if(is_null(stack)) {
        mem_error("Unable to heap allocate a production stack.");
    }

    stack->frame = NULL;
    stack->unused = NULL;

    return stack;
}

/**
 * Free a production stack.
 */
void P_Production_free(P_ProductionStack *stack) {
    P_StackFrame *frame,
                 *next;

    assert_not_null(stack);

    for(frame = stack->frame; is_not_null(frame); frame = next) {
        next = frame->caller;
        prod_mem_free(frame);
    }

    for(frame = stack->unused; is_not_null(frame); frame = next) {
        next = frame->caller;
        prod_mem_free(frame);
    }

    prod_mem_free(stack);
}

/**
 * Push a stack frame onto the frame stack.
 */
void P_production_push(P_ProductionStack *stack,
                       P_ProductionCache *cache,
                       P_TreeSet *all_trees,
                       PParserProduction *prod,
                       PTerminalTree *backtrack_point) {

    P_StackFrame *frame = NULL;

    /* make a new frame or re-use an existing but unused one */
    if(is_null(stack->unused)) {
        frame = prod_mem_alloc(sizeof(P_StackFrame));
        if(is_null(frame)) {
            mem_error("Unable to allocate new parser stack frame on the heap.");
        }
    } else {
        frame = stack->unused;
        stack->unused = frame->caller;
    }

    /* initialize the frame */
    frame->caller = stack->frame;
    frame->production = prod;
    frame->backtrack_point = backtrack_point;
    frame->do_backtrack = 0;
    frame->curr_rule_list = gen_list_get_elm(prod->alternatives);
    frame->alternative_rules = (PGenericList *) list_get_next(
        (PList *) prod->alternatives
    );
    frame->parse_tree = (PParseTree *) P_tree_alloc_non_terminal(
        prod->max_num_useful_rewrite_rules,
        prod->production
    );

    /* record this tree in the set of all trees */
    P_tree_set_add(all_trees, frame->parse_tree);

    /* store the default cache value, this is so that we only ever store cache
     * keys once. */
    P_cache_store_initial(
        cache,
        prod->production,
        frame->backtrack_point
    );

    /* push it on to the stack */
    stack->frame = frame;

    return;
}

/**
 * Pop a stack frame off of the frame stack and put it into the unused stack
 * where it will still be accessible.
 */
static void P_production_pop(P_ProductionStack *stack) {
    P_StackFrame *frame;

    assert_not_null(stack);
    assert_not_null(stack->frame);

    frame = stack->frame;
    stack->frame = frame->caller;

    frame->caller = stack->unused;
    stack->unused = frame;

    return;
}

/**
 * Pop a production off of the stack and also merge its parse tree into the
 * the parent production's parse tree.
 */
void P_production_succeeded(P_ProductionStack *stack,
                            P_ProductionCache *cache,
                            PTerminalTree *end_point) {

    P_StackFrame *frame,
                 *caller;

    assert_not_null(stack);
    assert_not_null(cache);
    assert_not_null(stack->frame);
    assert_not_null((stack->frame)->caller);

    frame = stack->frame;
    caller = frame->caller;

    /* include the tree */
    P_tree_record_production(
        frame->parse_tree,
        caller->parse_tree,
        (PParserRewriteRule *) gen_list_get_elm(caller->curr_rule_list)
    );

    /* cache the success */
    P_cache_store_success(
        cache,
        (frame->production)->production,
        frame->backtrack_point,
        end_point,
        frame->parse_tree
    );

    /* pop off the production */
    P_production_pop(stack);

    /* go to the next rule */
    P_production_continue(stack);
}

/**
 * Record a failure of the production.
 */
void P_production_failed(P_ProductionStack *stack,
                         P_ProductionCache *cache) {
    assert_not_null(stack);
    assert_not_null(stack->frame);
    assert_not_null(cache);

    P_cache_store_failure(
        cache,
        ((stack->frame)->production)->production,
        (stack->frame)->backtrack_point
    );

    P_production_pop(stack);
}

/**
 * A cached production succeeded.
 */
void P_production_cache_succeeded(P_ProductionStack *stack,
                                  P_ProductionCacheValue *cached_result) {
    assert_not_null(stack);
    assert_not_null(cached_result);
    assert_not_null(stack->frame);

    /* record the cache tree */
    P_tree_record_production(
        P_cache_value_get_tree(cached_result),
        (stack->frame)->parse_tree,
        P_production_get_rule(stack)
    );

    /* advance to the next rewrite rule */
    P_production_continue(stack);
}

/**
 * The frame at the top of the frame stack succeeded at matching its
 * production.
 */
void P_production_continue(P_ProductionStack *stack) {
    assert_not_null(stack);
    assert_not_null(stack->frame);

    (stack->frame)->curr_rule_list = (PGenericList *) list_get_next(
        (PList *) (stack->frame)->curr_rule_list
    );
}

/**
 * The frame at the top of the frame stack failed to match something and hence
 * needs to backtrack.
 */
void P_production_break(P_ProductionStack *stack) {
    assert_not_null(stack);
    assert_not_null(stack->frame);
    (stack->frame)->do_backtrack = 1;
}

/**
 * Check if the production on the top of the stack needs to backtrack.
 */
int P_production_rule_failed(P_ProductionStack *stack) {
    assert_not_null(stack);
    assert_not_null(stack->frame);
    return (stack->frame)->do_backtrack;
}

/**
 * Check if there are no production frames left.
 */
int P_production_can_cascade(P_ProductionStack *stack) {
    assert_not_null(stack);
    return is_not_null(stack->frame);
}

/**
 * Check if the top production on the stack is the only production left on the
 * stack.
 */
int P_production_is_root(P_ProductionStack *stack) {
    assert_not_null(stack);
    assert_not_null(stack->frame);
    return is_null((stack->frame)->caller);
}

/**
 * The frame on the top of the stack failed to match one of the rule sequences
 * of its production, perform the backtracking operation.
 */
PTerminalTree *P_production_backtrack(P_ProductionStack *stack) {
    P_StackFrame *frame;

    assert_not_null(stack);
    assert_not_null(stack->frame);

    frame = stack->frame;

    /* advance to the next rule sequence */
    frame->curr_rule_list = (PGenericList *) gen_list_get_elm(
        frame->alternative_rules
    );
    frame->alternative_rules = (PGenericList *) list_get_next(
        (PList *) frame->alternative_rules
    );

    /* prevent a cascade */
    frame->do_backtrack = 0;

    /* drop the children off of this parse tree */
    tree_clear((PTree *) frame->parse_tree, 1);

    return frame->backtrack_point;
}

/**
 * Check whether or not a production can backtrack.
 */
int P_production_can_backtrack(P_ProductionStack *stack) {
    assert_not_null(stack);
    assert_not_null(stack->frame);

    return is_not_null((stack->frame)->alternative_rules);
}

/**
 * Get the current rule from the frame.
 */
PParserRewriteRule *P_production_get_rule(P_ProductionStack *stack) {
    assert_not_null(stack);
    assert_not_null(stack->frame);
    assert_not_null((stack->frame)->curr_rule_list);
    return (PParserRewriteRule *) gen_list_get_elm((stack->frame)->curr_rule_list);
}

/**
 * Check if the frame has a next rule.
 */
int P_production_has_rule(P_ProductionStack *stack) {
    assert_not_null(stack);
    assert_not_null(stack->frame);
    return is_not_null((stack->frame)->curr_rule_list);
}

/**
 * Get the parse tree in construction for the top production on the production
 * stack.
 */
PParseTree *P_production_get_parse_tree(P_ProductionStack *stack) {
    assert_not_null(stack);

    if(is_null(stack->frame)) {
        assert_not_null(stack->unused);

        return (stack->unused)->parse_tree;
    }
    return (stack->frame)->parse_tree;
}
