/*
 * dfa.c
 *
 *  Created on: Jun 26, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-nfa.h>

#define NFA_NUM_DEFAULT_STATES 256
#define NFA_NUM_DEFAULT_TRANSITIONS 256
#define NFA_NUM_DEFAULT_STATE_TRANSITIONS 4

typedef struct NFA_Transition {
    enum {
        T_VALUE,
        T_SET,
        T_EPSILON
    } type;
    union {
        unsigned int value;
        PSet *set;
    } condition;
    unsigned int to_state;
} NFA_Transition;

typedef struct NFA_State {

    unsigned int num_transitions,
                 num_transition_slots,
                 id,
                 is_accepting;
    NFA_Transition **transitions;
} NFA_State;

/* -------------------------------------------------------------------------- */

static void NFA_alloc_slot(void **slots,
                           unsigned int *num_entries,
                           unsigned int *num_slots,
                           size_t slot_size) {

    unsigned int slot_count = *num_slots;

    if(*num_entries >= slot_count) {
        if(slot_count > (((unsigned int) -1) / 2)) {
            slot_count = (unsigned int) -1;
        } else {
            slot_count *= 2;
        }
        *slots = mem_realloc(
            *slots,
            slot_size * slot_count
        );
        if(is_null(*slots)) {
            mem_error("Unable to grow a NFA table.");
        }
    }

    *num_slots = slot_count;
    ++(*num_entries);
}

static NFA_Transition *NFA_alloc_transition(PNFA *nfa,
                                            unsigned int start_state,
                                            unsigned int end_state) {
    NFA_Transition *trans;
    NFA_State *state;
    void *transitions;

    assert_not_null(nfa);
    assert(start_state < nfa->num_states);
    assert(end_state < nfa->num_states);

    NFA_alloc_slot(
        &nfa->transitions,
        &nfa->num_transitions,
        &nfa->num_transition_slots,
        sizeof(NFA_Transition)
    );

    trans = ((NFA_Transition *) nfa->transitions) + (nfa->num_transitions - 1);
    trans->to_state = end_state;

    /* got add this transition to the starting state */
    state = ((NFA_State *) nfa->states) + start_state;
    transitions = (void *) state->transitions;

    NFA_alloc_slot(
        &transitions,
        &state->num_transitions,
        &state->num_transition_slots,
        sizeof(NFA_Transition *)
    );
    state->transitions[state->num_transitions - 1] = trans;

    return trans;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a new non-deterministic finite automata on the heap. This default
 * starting state is 0.
 */
PNFA *nfa_alloc(void) {

    PNFA *nfa;
    NFA_Transition *transitions;
    NFA_State *states;

    nfa = mem_alloc(sizeof(PNFA));
    transitions = mem_alloc(sizeof(NFA_Transition) * NFA_NUM_DEFAULT_TRANSITIONS);
    states = mem_alloc(sizeof(NFA_State) * NFA_NUM_DEFAULT_STATES);

    if(is_null(nfa) || is_null(transitions) || is_null(states)) {
        mem_error("Unable to allocate DFA on the heap.");
    }

    nfa->current_state = 0;
    nfa->num_state_slots = NFA_NUM_DEFAULT_STATES;
    nfa->num_transition_slots = NFA_NUM_DEFAULT_TRANSITIONS;
    nfa->num_states = 0;
    nfa->num_transitions = 0;
    nfa->states = (void **) states;
    nfa->transitions = (void **) transitions;

    return nfa;
}

/**
 * Free a NFA.
 */
void nfa_free(PNFA *nfa) {
    unsigned int i;
    NFA_State *state;

    assert_not_null(nfa);

    for(state = nfa->states, i = nfa->num_states; i--; ++state) {
        mem_free(state->transitions);
    }

    mem_free(nfa->transitions);
    mem_free(nfa->states);
    mem_free(nfa);
}

/**
 * Change the starting state of the NFA to start_state.
 */
void nfa_change_start_state(PNFA *nfa, unsigned int start_state) {
    assert_not_null(nfa);
    assert(start_state < nfa->num_states);
    nfa->start_state = start_state;
}

/**
 * Add a state to the NFA.
 */
unsigned int nfa_add_state(PNFA *nfa, int is_accepting) {
    NFA_State *state;
    assert_not_null(nfa);

    NFA_alloc_slot(
        &nfa->states,
        &nfa->num_states,
        &nfa->num_state_slots,
        sizeof(NFA_State)
    );

    state = ((NFA_State *) nfa->states) + (nfa->num_states - 1);
    state->id = nfa->num_states - 1;
    state->is_accepting = is_accepting;
    state->num_transition_slots = NFA_NUM_DEFAULT_STATE_TRANSITIONS;
    state->num_transitions = 0;
    state->transitions = mem_alloc(
        NFA_NUM_DEFAULT_STATE_TRANSITIONS * sizeof(NFA_Transition *)
    );

    return state->id;
}

/**
 * Add an epsilon transition to the NFA starting from start_state an going to
 * end_state. These transitions are taken automatically.
 */
void nfa_add_epsilon_transition(PNFA *nfa,
                                unsigned int start_state,
                                unsigned int end_state) {
    NFA_Transition *trans = NFA_alloc_transition(nfa, start_state, end_state);
    trans->type = T_EPSILON;
}

/**
 * Add a value transition to the NFA starting from start_state an going to
 * end_state. These transitions are taken if their value corresponds with the
 * expected value.
 */
void nfa_add_value_transition(PNFA *nfa,
                              unsigned int start_state,
                              unsigned int end_state,
                              unsigned int test_value) {
    NFA_Transition *trans = NFA_alloc_transition(nfa, start_state, end_state);
    trans->type = T_VALUE;
    trans->condition.value = test_value;
}

/**
 * Add a set transition to the NFA starting from start_state an going to
 * end_state. These transitions are taken if the expected value is a member
 * of the set test_set.
 */
void nfa_add_set_transition(PNFA *nfa,
                            unsigned int start_state,
                            unsigned int end_state,
                            PSet *test_set) {
    NFA_Transition *trans = NFA_alloc_transition(nfa, start_state, end_state);
    trans->type = T_SET;
    trans->condition.set = test_set;
}

/**
 * Print out the NFA in the DOT language.
 */
void nfa_print_dot(PNFA *nfa) {

    unsigned int i,
                 j;

    NFA_State *state;
    NFA_Transition *transition;

    assert_not_null(nfa);

    for(state = nfa->states, i = nfa->num_states; i--; ++state) {

        if(state->is_accepting) {
            printf( "Ox%d [label=%d shape=doublecircle] \n", state->id, state->id);
        } else {
            printf("Ox%d [label=%d] \n", state->id, state->id);
        }

        if(state->id == nfa->start_state) {
            printf("Ox%d [color=green] \n", state->id);
        }

        for(j = 0; j < state->num_transitions; ++j) {
            transition = state->transitions[j];

            switch(transition->type) {
                case T_VALUE:
                    printf(
                        "Ox%d -> Ox%d [label=\" '%c' \"] \n",
                        state->id,
                        transition->to_state,
                        (unsigned char) transition->condition.value
                    );
                    break;
                case T_SET:
                    printf(
                        "Ox%d -> Ox%d [label=\"{ ... }\"] \n",
                        state->id,
                        transition->to_state
                    );
                    break;
                case T_EPSILON:
                    printf(
                        "Ox%d -> Ox%d [label=\" '' \"] \n",
                        state->id,
                        transition->to_state
                    );
                    break;
            }
        }
    }
}
