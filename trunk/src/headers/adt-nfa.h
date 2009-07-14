/*
 * adt-dfa.h
 *
 *  Created on: Jun 26, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef ADTNFA_H_
#define ADTNFA_H_

#include <string.h>

#include "adt-dict.h"
#include "adt-set.h"
#include "func-delegate.h"

#define NFA_MAX_KNOWN_UNUSED_STATES 64
#define NFA_NUM_DEFAULT_TRANSITIONS 256

typedef enum {
    T_VALUE,
    T_SET,
    T_EPSILON,
    T_UNUSED
} NFA_TransitionType;

typedef struct NFA_Transition {

    NFA_TransitionType type;

    union {
        int value;
        PSet *set;
    } condition;

    unsigned int from_state,
                 to_state,
                 id;

    struct NFA_Transition *trans_next,
                          *dest_next;
} NFA_Transition;

typedef struct NFA_TransitionGroup {
    struct NFA_TransitionGroup *next;
    NFA_Transition transitions[NFA_NUM_DEFAULT_TRANSITIONS];
    unsigned int num_transitions;

} NFA_TransitionGroup;

typedef struct PNFA {

    unsigned int num_states,
                 num_transitions,
                 num_state_slots,
                 start_state,
                 current_state,
                 num_unused_states,
                 unused_states[NFA_MAX_KNOWN_UNUSED_STATES];

    int *conclusions;

    NFA_TransitionGroup *transition_group;
    NFA_Transition **state_transitions,
                   **destination_states;

    PSet *accepting_states;
} PNFA;

PNFA *nfa_alloc(void);

void nfa_free(PNFA *nfa);

PNFA *nfa_to_dfa(PNFA *nfa, PSet *priority_set);

void nfa_change_start_state(PNFA *nfa, unsigned int start_state);

unsigned int nfa_add_state(PNFA *nfa);

void nfa_add_accepting_state(PNFA *nfa, unsigned int which_state);

void nfa_add_conclusion(PNFA *nfa, unsigned int which_state, int conclusion);

void nfa_merge_states(PNFA *nfa, unsigned int state_a, unsigned int state_b);

void nfa_add_epsilon_transition(PNFA *nfa,
                                unsigned int start_state,
                                unsigned int end_state);

void nfa_add_value_transition(PNFA *nfa,
                              unsigned int start_state,
                              unsigned int end_state,
                              int test_value);

void nfa_add_set_transition(PNFA *nfa,
                            unsigned int start_state,
                            unsigned int end_state,
                            PSet *test_set);

/* -------------------------------------------------------------------------- */

void nfa_print_dot(PNFA *nfa);

/* -------------------------------------------------------------------------- */

void nfa_print_scanner(const PNFA *nfa,
                       const char *out_file,
                       const char *func_name);

#endif /* ADTDFA_H_ */
