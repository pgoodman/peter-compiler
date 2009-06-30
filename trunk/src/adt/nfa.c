/*
 * Simple Non-deterministic Finite Automata library. This library was primarily
 * design to work with a NFA constructed using Thompson's Construction. Because
 * of this, certain guarantees are expected to hold when operations such as
 * merging states is done.
 *
 *  Created on: Jun 26, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <adt-nfa.h>

#define D(x)

#define NFA_NUM_DEFAULT_STATES 256
#define NFA_NUM_DEFAULT_TRANSITIONS 256
#define NFA_NUM_DEFAULT_STATE_TRANSITIONS 4
#define NFA_MAX_EPSILON_STACK 256
#define NFA_UNUSED_STATE ((void *) 0x1)

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
    struct NFA_Transition *trans_next,
                          *dest_next;
} NFA_Transition;

/* -------------------------------------------------------------------------- */

static void NFA_alloc_slot(void **slots,
                           unsigned int *num_entries,
                           unsigned int *num_slots,
                           size_t slot_size,
                           int inc_sizes) {

    unsigned int old_slot_count = *num_slots,
                 new_slot_count = old_slot_count;
    int c;
    char *slot;

    if(*num_entries >= old_slot_count) {

        if(old_slot_count > (((unsigned int) -1) / 2)) {
            new_slot_count = (unsigned int) -1;
        } else {
            new_slot_count *= 2;
        }

        *slots = mem_realloc(
            *slots,
            slot_size * new_slot_count
        );

        if(is_null(*slots)) {
            mem_error("Unable to grow a NFA table.");
        }

        /* zero out the new slots */
        c = (new_slot_count - old_slot_count) * slot_size;
        slot = ((char *) *slots) + (old_slot_count *new_slot_count);
        for(; --c >= 0; ++slot) {
            slot = 0;
        }
    }

    if(inc_sizes) {
        *num_slots = new_slot_count;
        ++(*num_entries);
    }
}

static NFA_Transition *NFA_alloc_transition(PNFA *nfa,
                                            unsigned int start_state,
                                            unsigned int end_state) {
    NFA_Transition *trans,
                   *prev;
    void *transitions;

    assert_not_null(nfa);
    assert(start_state < nfa->num_states);
    assert(end_state < nfa->num_states);

    NFA_alloc_slot(
        &nfa->transitions,
        &nfa->num_transitions,
        &nfa->num_transition_slots,
        sizeof(NFA_Transition),
        1
    );

    trans = ((NFA_Transition *) nfa->transitions) + (nfa->num_transitions - 1);
    trans->to_state = end_state;
    trans->trans_next = NULL;
    trans->dest_next = NULL;

    /* first transition being added on the start state. */
    if(is_not_null(nfa->state_transitions[start_state])) {
        trans->trans_next = nfa->state_transitions[start_state];
    }

    if(is_not_null(nfa->destination_states[end_state])) {
        trans->dest_next = nfa->destination_states[end_state];
    }

    nfa->state_transitions[start_state] = trans;
    nfa->destination_states[end_state] = trans;

    return trans;
}

typedef struct {
    unsigned int bottom[NFA_MAX_EPSILON_STACK],
                 *ptr,
                 *top;
} NFA_EpsilonStack;

/**
 * Mapping function used to add states to the NFA epsilon closure stack.
 */
static void NFA_state_map(NFA_EpsilonStack *stack, unsigned int state) {
    if(stack->ptr >= stack->top) {
        std_error("Internal Error: Unable to complete e-closure for NFA.");
    }
    D( printf("\t\t\t Transition set contains state %d \n", state); )

    *stack->ptr = state;
    ++stack->ptr;
}

/**
 * Find the transitive closure of epsilon transitions on a particular set of
 * states. The epsilon closure for the set of states is added to the set of
 * states in place.
 *
 * Returns if the DFA state represented by the e-closure has an NFA state that
 * is accepting.
 */
static int NFA_epsilon_closure(PNFA *nfa,
                               PSet *states) {

    NFA_EpsilonStack stack;
    NFA_Transition *transition;

    int is_accepting = 0,
        i;

    stack.ptr = stack.bottom;
    stack.top = stack.bottom + NFA_MAX_EPSILON_STACK;

    if(is_null(states) || !set_cardinality(states)) {
        return 0;
    }

    D( printf("\t finding epsilon closure... \n"); )

    /* add the states in the set to the stack */
    set_map(states, (void *) &stack, (PSetMap *) NFA_state_map);

    while(stack.ptr > stack.bottom && stack.ptr < stack.top) {

        transition = nfa->state_transitions[*(--stack.ptr)];

        if(set_has_elm(nfa->accepting_states, *stack.ptr)) {
            D( printf("\t\t\t %d is an accepting state. \n", *stack.ptr); )
            is_accepting = 1;
        }

        for(; is_not_null(transition); transition = transition->trans_next) {

            D( printf("\t\t in %p. \n", (void *) transition); )

            if(transition->type == T_EPSILON
            && !set_has_elm(states, transition->to_state)) {

                if(stack.ptr >= stack.top) {
                    std_error("Internal Error: Unable to continue e-closure.");
                }

                set_add_elm(states, transition->to_state);
                *(stack.ptr++) = transition->to_state;

                D( printf(
                    "\t\t\t Epsilon closure contains state %d \n",
                    transition->to_state
                ); )
            }

            D( printf("\t\t out. \n"); )
        }
    }

    D( printf(
        "\t done, epsilon closure has %d NFA states. \n",
        set_cardinality(states)
    ); )

    return is_accepting;
}

/**
 * Simulate transitions from the states in the input_set over an input from our
 * alphabet. Return the set of states that we successfully transitioned to from
 * the set of input states.
 */
static PSet *NFA_simulate_transition(PNFA *nfa,
                                     PSet *input_set,
                                     unsigned int input) {

    PSet *output_set = NULL;

    unsigned int state = 0;
    int i = nfa->num_states;

    NFA_Transition *transition,
                   **state_transitions = (NFA_Transition **) nfa->state_transitions;

    D( printf("\t simulating transition on ASCII %d... \n", input); )

    for(; --i >= 0; ++state) {

        if(!set_has_elm(input_set, state)) {
            continue;
        }

        transition = *(((NFA_Transition **) state_transitions) + state);

        for(; is_not_null(transition);
            transition = transition->trans_next) {

            if(transition->type == T_VALUE) {
                if(transition->condition.value == input) {

                    D( printf(
                        "\t\t transition found on '%c', adding state %d \n",
                        (char) input,
                        transition->to_state
                    ); )

                    if(is_null(output_set)) {
                        output_set = set_alloc();
                    }

                    set_add_elm(output_set, transition->to_state);
                }
            } else if(transition->type == T_SET) {
                if(set_has_elm(transition->condition.set, input)) {

                    D( printf(
                        "\t\t transition found on '%c', adding state %d \n",
                        (char) input,
                        transition->to_state
                    ); )

                    if(is_null(output_set)) {
                        output_set = set_alloc();
                    }

                    set_add_elm(output_set, transition->to_state);
                }
            }
        }
    }

    D( printf("\t done, number of transitions %d. \n", set_cardinality(output_set)); )

    return output_set;
}

/**
 * Determine the largest character of the alphabet that this dfa recognizes.
 */
static unsigned int NFA_max_alphabet_char(PNFA *nfa) {

    unsigned int max = 0;

    NFA_Transition *trans = (NFA_Transition *) nfa->transitions;
    unsigned int i = nfa->num_transitions,
                 j;
    D( printf("\t finding the maximum alphabet character... \n"); )

    for(; i--; ++trans) {
        if(trans->type == T_VALUE) {
            if(trans->condition.value > max) {
                max = trans->condition.value;
            }
        } else if(trans->type == T_SET) {
            j = set_max_elm(trans->condition.set);
            if(j > max) {
                max = j;
            }
        }
    }

    D( printf("\t done, max char is ASCII %d. \n", max); )

    return max;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a new non-deterministic finite automata on the heap. This default
 * starting state is 0.
 */
PNFA *nfa_alloc(void) {

    PNFA *nfa;
    NFA_Transition *transitions,
                   **state_transitions,
                   **transition_destinations;

    nfa = mem_alloc(sizeof(PNFA));
    transitions = mem_calloc(
        NFA_NUM_DEFAULT_TRANSITIONS,
        sizeof(NFA_Transition)
    );
    state_transitions = mem_calloc(
        NFA_NUM_DEFAULT_STATES,
        sizeof(NFA_Transition *)
    );
    transition_destinations = mem_calloc(
        NFA_NUM_DEFAULT_STATES,
        sizeof(NFA_Transition *)
    );

    if(is_null(nfa)
    || is_null(transitions)
    || is_null(state_transitions)
    || is_null(transition_destinations)) {
        mem_error("Unable to allocate DFA on the heap.");
    }

    nfa->accepting_states = set_alloc();
    nfa->current_state = 0;
    nfa->num_state_slots = NFA_NUM_DEFAULT_STATES;
    nfa->num_transition_slots = NFA_NUM_DEFAULT_TRANSITIONS;
    nfa->num_states = 0;
    nfa->num_transitions = 0;
    nfa->state_transitions = (void **) state_transitions;
    nfa->destination_states = (void **) transition_destinations;
    nfa->transitions = (void *) transitions;
    nfa->num_unused_states = 0;

    return nfa;
}

/**
 * Free a NFA.
 */
void nfa_free(PNFA *nfa) {
    unsigned int i;
    NFA_Transition *transition;

    assert_not_null(nfa);

    i = nfa->num_transitions;
    for(transition = nfa->transitions; i--; ++transition) {
        if(transition->type == T_SET) {
            set_free(transition->condition.set);
        }
    }

    set_free(nfa->accepting_states);
    mem_free(nfa->transitions);
    mem_free(nfa->state_transitions);
    mem_free(nfa->destination_states);
    mem_free(nfa);
}

/**
 * Perform the subset construction on the NFA to turn it into a DFA.
 */
typedef struct DFA_State {
    PSet *nfa_states;
    unsigned int id;
} DFA_State;

PNFA *nfa_to_dfa(PNFA *nfa) {

    unsigned int next_state_id,
                 prev_state_id,
                 num_dfa_states = 0,
                 num_dfa_slots = NFA_NUM_DEFAULT_STATES;

    int largest_char = NFA_max_alphabet_char(nfa) + 1,
        c,
        is_accepting;

    DFA_State *dfa_state_stack,
              *state,
              *new_state;

    /* this maps the state sets to the DFA dfa_state_stack indexes. Dict expects
     * pointer entries, but we will just give it ints as those are really what
     * we care about. */
    PDictionary *dfa_states = dict_alloc(
        (uint32_t) NFA_NUM_DEFAULT_STATES,
        (PDictionaryHashFunc *) &set_hash,
        (PDictionaryCollisionFunc *) &set_not_equals
    );

    PNFA *dfa = nfa_alloc();

    PSet *transition_set,
         *nfa_state_set;

    D( printf("starting. \n"); )

    /* this stack holds all DFA state information. the set of NFA states
     * represented by a DFA state is only meaningful in the context of DFA
     * construction and so it is stored here instead of elsewhere. */
    dfa_state_stack = mem_alloc(sizeof(DFA_State) * num_dfa_slots);
    if(is_null(dfa_state_stack)) {
        mem_error("Internal Error: Unable to begin converting NFA to DFA. \n");
    }

    /* start everything off by finding the epsilon closure of the starting
     * state of the NFA. the starting state is simultaneously in all states
     * within the epsilon closure of itself, and so the set of those states
     * represents a DFA state. */
    dfa_state_stack->id = nfa_add_state(dfa);
    dfa_state_stack->nfa_states = set_alloc();

    nfa_change_start_state(dfa, dfa_state_stack->id);
    set_add_elm(dfa_state_stack->nfa_states, nfa->start_state);

    D(
       printf(
           "starting state set has %d \n",
           set_cardinality(dfa_state_stack->nfa_states)
       );
    )

    if(NFA_epsilon_closure(nfa, dfa_state_stack->nfa_states)) {
        nfa_add_accepting_state(dfa, dfa_state_stack->id);
    }

    /* state id +1 is stored so that we can distinguish NULL from a state id.
     * i.e. we could use dict_is_set to check if a state exists, but that means
     * doing a dict_get which might repeat some costly lookup. state 0 would
     * otherwise be stored as a null pointer, and if a dict entry doesn't exist
     * and dict_get is called then a null pointer is returned, so +1 to the
     * state id casted to void * lets us distinguish from no entry and an entry.
     */
    dict_set(
         dfa_states,
         dfa_state_stack->nfa_states,
         (void *) (dfa_state_stack->id + 1),
         &delegate_do_nothing
    );

    ++num_dfa_states;

    while(num_dfa_states > 0 && num_dfa_states < num_dfa_slots) {

        /* take the top state off of the stack and use it. */
        state = dfa_state_stack + --num_dfa_states;
        prev_state_id = state->id;
        nfa_state_set = state->nfa_states;

        D( printf("Simulating state transitions... \n"); )

        for(c = largest_char; --c >= 0; ) {

            /* for each state in state->nfa_states, attempt to transition on the
             * input c and if a transition on c exists then add the destination
             * state to the return set. */
            transition_set = NFA_simulate_transition(nfa, nfa_state_set, c);
            if(!set_cardinality(transition_set)) {
                set_free(transition_set);
                continue;
            }

            /* for each state in the transition set, find all of the states that
             * they are simultaneously at by adding the transitive closure of
             * epsilon transitions to the transition_set. */
            is_accepting = 0;
            if(NFA_epsilon_closure(nfa, transition_set)) {
                is_accepting = 1;
            }

            /* we have already seen this DFA state */
            next_state_id = (unsigned int) dict_get(dfa_states, transition_set);

            if(next_state_id > 0) {
                set_free(transition_set);

            /* this is a new DFA state to add. */
            } else {

                if(num_dfa_states >= NFA_NUM_DEFAULT_STATES) {
                    std_error(
                        "Internal Error: Unable to continue subset construction."
                    );
                }

                next_state_id = nfa_add_state(dfa);
                state = dfa_state_stack + num_dfa_states;
                state->id = next_state_id;
                state->nfa_states = transition_set;

                if(is_accepting) {
                    nfa_add_accepting_state(dfa, next_state_id);
                }

                /* store the DFA state's state id in the dict, keyed by the
                 * subset of NFA states represented by this DFA state. */
                dict_set(
                    dfa_states,
                    transition_set,
                    (void *) ++next_state_id,
                    &delegate_do_nothing
                );

                ++num_dfa_states;
            }

            /* add in the new transition */
            nfa_add_value_transition(
                dfa,
                prev_state_id,
                next_state_id - 1, /* -1 because we store state id+1 */
                c
            );
        }

        D( printf("done. \n"); )
    }

clean_up:

    D( printf("cleaning up. \n"); )

    dict_free(
        dfa_states,
        (PDictionaryFreeKeyFunc *) &set_free,
        &delegate_do_nothing
    );

    mem_free(dfa_state_stack);

    D( printf("done. \n\n"); )

    return dfa;
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
unsigned int nfa_add_state(PNFA *nfa) {

    void *transitions,
         *destinations;
    unsigned int state;

    assert_not_null(nfa);

    if(nfa->num_unused_states == 0) {

        transitions = (void *) nfa->state_transitions;
        destinations = (void *) nfa->destination_states;

        NFA_alloc_slot(
            &destinations,
            &nfa->num_states,
            &nfa->num_state_slots,
            sizeof(NFA_Transition *),
            0
        );
        NFA_alloc_slot(
            &transitions,
            &nfa->num_states,
            &nfa->num_state_slots,
            sizeof(NFA_Transition *),
            1
        );

        state = nfa->num_states - 1;
    } else {
        state = nfa->unused_states[--(nfa->num_unused_states)];
    }

    nfa->state_transitions[state] = NULL;
    nfa->destination_states[state] = NULL;
    return state;
}

/**
 * Make a state into an accepting state.
 */
void nfa_add_accepting_state(PNFA *nfa, unsigned int which_state) {
    assert_not_null(nfa);
    assert(which_state < nfa->num_states);
    set_add_elm(nfa->accepting_states, which_state);
}

/**
 * Merge state a and state b into state a by transferring all of b's transitions
 * to a and also transferring any transitions going to b to going into a.
 */
void nfa_merge_states(PNFA *nfa, unsigned int state_a, unsigned int state_b) {

    NFA_Transition *trans,
                   **state_trans,
                   **dest_trans;

    assert_not_null(nfa);
    assert(state_a < nfa->num_states);
    assert(state_b < nfa->num_states);

    state_trans = (NFA_Transition **) nfa->state_transitions;
    dest_trans = (NFA_Transition **) nfa->destination_states;

    /* move transitions leaving b into a */
    if(is_null(state_trans[state_a])) {
        state_trans[state_a] = state_trans[state_b];
    } else {
        for(trans = state_trans[state_a];
            is_not_null(trans->trans_next);
            trans = trans->trans_next)
            ;
        trans->trans_next = state_trans[state_b];
    }

    /* move transitions going into b into a */
    if(is_not_null(dest_trans[state_b])) {
        for(trans = dest_trans[state_b];
            is_not_null(trans->dest_next);
            trans = trans->dest_next) {
            trans->to_state = state_a;
        }

        trans->dest_next = dest_trans[state_a];
        trans->to_state = state_a;
        dest_trans[state_a] = trans;
    }

    /* collect state b to be reused */
    state_trans[state_b] = NFA_UNUSED_STATE;
    dest_trans[state_b] = NULL;
    if(nfa->num_unused_states < NFA_MAX_KNOWN_UNUSED_STATES) {
        nfa->unused_states[(nfa->num_unused_states)++] = state_b;
    }
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

    unsigned int i = nfa->num_states,
                 state = 0;

    NFA_Transition *transition,
                   **state_transitions = (NFA_Transition **) nfa->state_transitions;

    assert_not_null(nfa);

    for(state = 0; i--; ++state) {
        transition = *(((NFA_Transition **) state_transitions) + state);

        if(transition == NFA_UNUSED_STATE) {
            continue;
        }

        if(set_has_elm(nfa->accepting_states, state)) {
            printf(
                "Ox%d [label=<S<FONT POINT-SIZE=\"8\"> %d</FONT>> "
                "shape=doublecircle] \n",
                state,
                state
            );
        } else {
            printf(
                "Ox%d [label=<S<FONT POINT-SIZE=\"8\"> %d</FONT>>] \n",
                state,
                state
            );
        }

        if(state == nfa->start_state) {
            printf("Ox%d [shape=polygon] \n", state);
        }

        for(; is_not_null(transition); transition = transition->trans_next) {
            switch(transition->type) {
                case T_VALUE:
                    printf(
                        "Ox%d -> Ox%d [label=\" '%c'  \"] \n",
                        state,
                        transition->to_state,
                        (unsigned char) transition->condition.value
                    );
                    break;
                case T_SET:
                    printf(
                        "Ox%d -> Ox%d [label=\" { ... }  \"] \n",
                        state,
                        transition->to_state
                    );
                    break;
                case T_EPSILON:
                    printf(
                        "Ox%d -> Ox%d [label=< &#949;  >] \n",
                        state,
                        transition->to_state
                    );
                    break;
            }
        }
    }
}
