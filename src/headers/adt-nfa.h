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

typedef struct PNFA {
    unsigned int num_states,
                 num_transitions,
                 num_state_slots,
                 num_transition_slots,
                 start_state,
                 current_state,
                 num_unused_states,
                 unused_states[NFA_MAX_KNOWN_UNUSED_STATES];
    void *transitions,
         **state_transitions,
         **destination_states;

    PSet *accepting_states;
} PNFA;

PNFA *nfa_alloc(void);

void nfa_free(PNFA *nfa);

PNFA *nfa_to_dfa(PNFA *nfa);

void nfa_change_start_state(PNFA *nfa, unsigned int start_state);

unsigned int nfa_add_state(PNFA *nfa);

void nfa_add_accepting_state(PNFA *nfa, unsigned int which_state);

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

void nfa_print_dot(PNFA *nfa);

#endif /* ADTDFA_H_ */
