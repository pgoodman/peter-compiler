/*
 * adt-dfa.h
 *
 *  Created on: Jun 26, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef ADTNFA_H_
#define ADNFA_H_

#include "adt-set.h"

typedef struct PNFA {
    unsigned int num_states,
                 num_transitions,
                 num_state_slots,
                 num_transition_slots,
                 start_state,
                 current_state;
    void *states,
         *transitions;
} PNFA;

PNFA *nfa_alloc(void);

void nfa_free(PNFA *nfa);

void nfa_change_start_state(PNFA *nfa, unsigned int start_state);

unsigned int nfa_add_state(PNFA *nfa, int is_accepting);

void nfa_add_epsilon_transition(PNFA *nfa,
                                unsigned int start_state,
                                unsigned int end_state);

void nfa_add_value_transition(PNFA *nfa,
                              unsigned int start_state,
                              unsigned int end_state,
                              unsigned int test_value);

void nfa_add_set_transition(PNFA *nfa,
                            unsigned int start_state,
                            unsigned int end_state,
                            PSet *test_set);

void nfa_print_dot(PNFA *nfa);

#endif /* ADTDFA_H_ */
