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
#define NFA_NUM_DEFAULT_STATE_TRANSITIONS 4
#define NFA_MAX_EPSILON_STACK 256
#define NFA_UNUSED_STATE ((void *) 0x1)

static int LABEL_SUBSETS = 0;

int nfa_label_states(int set, int val) {
    if(set) {
        LABEL_SUBSETS = val;
    }
    return LABEL_SUBSETS;
}

/* -------------------------------------------------------------------------- */

/**
 * Generic slot allocation mechanism. This will grow the size of a slot space
 * by a factor of two.
 */
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
            mem_error("Internal NFA Error: Unable to grow a NFA table.");
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

/**
 * Allocate and return a new transition to be used in the NFA.
 */
static NFA_Transition *NFA_alloc_transition(PNFA *nfa,
                                            unsigned int start_state,
                                            unsigned int end_state) {
    NFA_Transition *trans;
    NFA_TransitionGroup *group;

    assert_not_null(nfa);
    assert(start_state < nfa->num_states);
    assert(end_state < nfa->num_states);

    if(nfa->transition_group->num_transitions >= NFA_NUM_DEFAULT_TRANSITIONS) {
        group = mem_calloc(1, sizeof(NFA_TransitionGroup));
        if(is_null(group)) {
            mem_error("Internal NFA Error: Unable to add more transitions.");
        }
        group->num_transitions = 0;
        group->next = nfa->transition_group;
        nfa->transition_group = group;
    }

    trans = nfa->transition_group->transitions
          + nfa->transition_group->num_transitions;

    ++(nfa->num_transitions);

    trans->from_state = start_state;
    trans->to_state = end_state;
    trans->trans_next = NULL;
    trans->dest_next = NULL;
    trans->id = nfa->num_transitions - 1;

    ++(nfa->transition_group->num_transitions);

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

/**
 * Mark a NFA state as unused.
 */
static void NFA_mark_unused_state(PNFA *nfa, unsigned state) {

    nfa->state_transitions[state] = NFA_UNUSED_STATE;
    nfa->destination_states[state] = NULL;
    if(nfa->num_unused_states < NFA_MAX_KNOWN_UNUSED_STATES) {
        nfa->unused_states[(nfa->num_unused_states)++] = state;
    }
}

/* fixed-size integer stack, generally used for keeping tracking of states. */
typedef struct {
    unsigned int bottom[NFA_MAX_EPSILON_STACK],
                 *ptr,
                 *top;
} NFA_StateStack;

/**
 * Mapping function used to add states to the NFA epsilon closure stack.
 */
static void NFA_state_stack_push(NFA_StateStack *stack, unsigned int state) {

    if(stack->ptr >= stack->top) {
        std_error("Internal Error: Unable to complete e-closure for NFA.");
    }

    D( printf("\t\t\t Mapped set contains state %d \n", state); )

    *stack->ptr = state;
    ++(stack->ptr);
}

/**
 * Determine the largest character of the alphabet that this dfa recognizes.
 */
static unsigned int NFA_max_alphabet_char(PNFA *nfa) {

    int max = 0,
        j,
        i = nfa->num_transitions;

    NFA_TransitionGroup *group;
    NFA_Transition *trans;

    D( printf("\t finding the maximum alphabet character... \n"); )

    for(group = nfa->transition_group; is_not_null(group); group = group->next) {
        trans = group->transitions;
        for(i = group->num_transitions; --i >= 0; ++trans) {
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
    }

    D( printf("\t done, max char is ASCII %d. \n", max); )

    return max;
}

/**
 * Find the transitive closure of epsilon transitions on a particular set of
 * states. The epsilon closure for the set of states is added to the set of
 * states in place.
 *
 * If the subset of NFA states contains at least one accepting state than any
 * one of those state ids can be returned, or -1 if no accepting state is an
 * element of the subset.
 */
static int NFA_transitive_closure(PNFA *nfa,
                                  PSet *states,
                                  PSet *priority_states,
                                  unsigned int transition_type) {
    NFA_StateStack stack;
    NFA_Transition *transition;

    int accepting_id = -1,
        accepting_was_priority = 0,
        state_is_priority = 0;

    unsigned int state_id;

    stack.ptr = stack.bottom;
    stack.top = stack.bottom + NFA_MAX_EPSILON_STACK;

    if(is_null(states) || !set_cardinality(states)) {
        return 0;
    }

    D( printf("\t finding epsilon closure... \n"); )

    /* add the states in the set to the stack */
    set_map(states, (void *) &stack, (PSetMapFunc *) &NFA_state_stack_push);

    while(stack.ptr > stack.bottom && stack.ptr < stack.top) {

        state_id = *(--stack.ptr);
        transition = nfa->state_transitions[state_id];

        /* we have found an accepting state. the following heuristic is in place
         * in the event that more than one accepting states are reachable for
         * the same input: we prefer accepting states with no outgoing
         * transitions, if we find two such states then we error. */
        if(set_has_elm(nfa->accepting_states, state_id)) {
            D( printf("\t\t\t %d is an accepting state. \n", *stack.ptr); )

            state_is_priority = set_has_elm(priority_states, state_id);
            if(accepting_id < 0) {
                accepting_id = state_id;
                accepting_was_priority = state_is_priority;
            } else {
                if(state_is_priority) {
                    if(accepting_was_priority) {
                        std_error(
                            "Internal NFA Error: Cannot resolve conflict in "
                            "subset construction. Two transition-less accepting "
                            "states can be reached given the same input."
                        );
                    }

                    accepting_id = state_id;
                    accepting_was_priority = state_is_priority;

                    D( printf(
                        "\t\t\t\t choosing %d as the accepting state.\n",
                        state_id
                    ); )
                }
            }
        }

        for(; is_not_null(transition); transition = transition->trans_next) {
            if(set_has_elm(states, transition->to_state)) {
                continue;
            }
            if(transition->type == transition_type) {
                set_add_elm(states, transition->to_state);
                NFA_state_stack_push(&stack, transition->to_state);
            }
        }
    }

    D( printf("\t done. \n"); )

    return accepting_id;
}

/**
 * Simulate transitions from the transitions on the states in input_states to
 * their respective states and then return that set of states.
 */
static PSet *NFA_simulate_transition(PNFA *nfa,
                                     PSet *input_states,
                                     int input) {

    int i,
        j = 0;

    PSet *output_states = NULL;
    NFA_Transition **transitions = (NFA_Transition **) nfa->state_transitions,
                   *trans;

    for(i = nfa->num_states; --i >= 0; ++j) {

        if(!set_has_elm(input_states, j)) {
            continue;
        }

        trans = transitions[j];
        for(; is_not_null(trans); trans = trans->trans_next) {
            if(trans->type == T_VALUE) {
                if(trans->condition.value == input) {
                    D( printf(
                        "\t\t transition found on '%c', adding state %d \n",
                        (char) input,
                        trans->to_state
                    ); )
                    if(is_null(output_states)) {
                        output_states = set_alloc();
                    }
                    set_add_elm(output_states, trans->to_state);
                }
            } else if(trans->type == T_SET) {
                if(set_has_elm(trans->condition.set, input)) {
                    D( printf(
                        "\t\t transition found on '%c', adding state %d \n",
                        (char) input,
                        trans->to_state
                    ); )
                    if(is_null(output_states)) {
                        output_states = set_alloc();
                    }
                    set_add_elm(output_states, trans->to_state);
                }
            }
        }
    }

    D( printf("\t done. \n"); )

    return output_states;
}

/* DFA state, represents a subset of all NFA states. */
typedef struct DFA_State {
    PSet *nfa_states;
    unsigned int id,
                 accepting_id;
} DFA_State;

static void *uint_to_pointer(unsigned int val) {
    union {
        unsigned int val;
        void *ptr;
    } trans;
    trans.val = 0;
    trans.ptr = NULL;
    trans.val = val;
    return trans.ptr;
}

/**
 * Perform the subset construction on the NFA to turn it into a DFA.
 */
static PNFA *NFA_subset_construction(PNFA *nfa,
                                     PSet *priority_set,
                                     int largest_char) {

    unsigned int next_state_id,
                 prev_state_id,
                 num_dfa_states = 0;

    int c,
        as;

    DFA_State dfa_state_stack[NFA_NUM_DEFAULT_STATES],
              *state;

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

    as = NFA_transitive_closure(
        nfa,
        dfa_state_stack->nfa_states,
        priority_set,
        T_EPSILON
    );

    if(as >= 0) {
        nfa_add_accepting_state(dfa, dfa_state_stack->id);
    }

    if(LABEL_SUBSETS) {
        vector_set(
            dfa->state_subsets,
            0,
            set_copy(dfa_state_stack->nfa_states),
            delegate_do_nothing
        );
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
         ((char *) NULL) + (dfa_state_stack->id + 1),
         &delegate_do_nothing
    );

    if(!set_cardinality(dfa_state_stack->nfa_states)) {
        goto clean_up;
    }

    ++num_dfa_states;
    ++largest_char; /* because of --c >= 0 below */

    while(num_dfa_states > 0 && num_dfa_states < NFA_NUM_DEFAULT_STATES) {

        /* take the top state off of the stack and use it. */
        state = dfa_state_stack + --num_dfa_states;
        prev_state_id = state->id;
        nfa_state_set = state->nfa_states;

        D( printf("Simulating state transitions... \n"); )

        /* -1 is for testing alpha-transitions */
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
            as = NFA_transitive_closure(
                nfa,
                transition_set,
                priority_set,
                T_EPSILON
            );

            /* we have already seen this DFA state */
            next_state_id = (unsigned int) (
                (char *) dict_get(dfa_states, transition_set) - (char *) NULL
            );

            if(next_state_id > 0) {
                --next_state_id;
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

                state->accepting_id = as;
                state->id = next_state_id;
                state->nfa_states = transition_set;

                /* store the DFA state's state id in the dict, keyed by the
                 * subset of NFA states represented by this DFA state. */
                dict_set(
                    dfa_states,
                    transition_set,
                    ((char *) NULL) + (next_state_id + 1),
                    &delegate_do_nothing
                );

                if(LABEL_SUBSETS) {
                    vector_set(
                        dfa->state_subsets,
                        next_state_id,
                        set_copy(transition_set),
                        delegate_do_nothing
                    );
                }

                ++num_dfa_states;
            }

            /* just make sure ;) */
            if(as >= 0) {
                nfa_add_accepting_state(dfa, next_state_id);
                nfa_add_conclusion(
                    dfa,
                    next_state_id,
                    *(nfa->conclusions + as)
                );
            }

            /* add in the new transition */
            nfa_add_value_transition(
                dfa,
                prev_state_id,
                next_state_id,
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

    D( printf("done. \n\n"); )

    return dfa;
}

/**
 * Minimize a DFA.
 */
#define AT(r,c,num_cols) (((r) * ((num_cols) + 1)) + (c))

static void collect_transitions(
    const unsigned state,
    const unsigned num_states,
    const int largest_char,
    NFA_Transition *trans,
    unsigned *destination_states,
    PSet *alphabet
) {
    int k;
    for(k = 0; k <= largest_char; ++k) {
        destination_states[k] = num_states;
    }

    for(; NULL != trans; trans = trans->trans_next) {
        assert(T_VALUE == trans->type);
        destination_states[trans->condition.value] = trans->to_state;
        set_add_elm(alphabet, trans->condition.value);
    }
}

struct mdfa_state_pair {
    PNFA *mdfa;
    unsigned state;
};

static void add_sink_state_transitions(struct mdfa_state_pair *pair, unsigned int label) {
    nfa_add_value_transition(
        pair->mdfa,
        pair->state,
        pair->mdfa->num_states - 1,
        (int) label
    );
}

static PNFA *DFA_minimize(PNFA *dfa, const int largest_char) {

    PNFA *mdfa;
    PSet *alphabet = set_alloc();
    PSet *seen_chars = NULL;
    PSet **state_subsets;
    char *distinguishable = NULL;
    unsigned *destination_states[2];
    unsigned num_states = dfa->num_states;
    unsigned state_i = 0, state_j = 0;
    unsigned *new_state_ids;
    unsigned *old_state_reps;
    unsigned at_i_j, at_j_i;
    unsigned state_i_is_accepting = 0;
    unsigned made_progress;
    NFA_Transition *trans;
    const unsigned i = 0, j = 1;
    unsigned k;
    unsigned need_sink_state = 0;
    struct mdfa_state_pair mdfa_and_state_id;

    /* scale the alphabet up */
    set_add_elm(alphabet, largest_char);

    distinguishable = mem_calloc(
        (num_states + 1) * (num_states + 1),
        sizeof(char)
    );

    destination_states[i] = mem_calloc(largest_char + 1, sizeof(int));
    destination_states[j] = mem_calloc(largest_char + 1, sizeof(int));

    /* fill in the last row and last column with 1. this is so that we can
     * refer to distinguishable states when checking the outgoing transitions
     * on two states. */
    for(k = 0; k <= num_states; ++k) {
        distinguishable[AT(k, num_states, num_states)] = 1;
        distinguishable[AT(num_states, k, num_states)] = 1;
    }
    distinguishable[AT(num_states, num_states, num_states)] = 0;

    D( printf("searching for distinguishable states %d.\n", num_states); )

    /* for every state i: */
    for(made_progress = 1; 1 == made_progress; ) {
        made_progress = 0;

        for(state_i = 0; state_i < num_states; ++state_i) {

            state_i_is_accepting = set_has_elm(dfa->accepting_states, state_i);

            collect_transitions(
                state_i,
                num_states,
                largest_char,
                dfa->state_transitions[state_i],
                destination_states[i],
                alphabet
            );

            /* for every state j: */
            for(state_j = 0; state_j < num_states; ++state_j) {

                /* same state */
                if(state_i == state_j) {
                    continue;
                }

                at_i_j = AT(state_i, state_j, num_states);
                at_j_i = AT(state_j, state_i, num_states);

                /* already marked */
                if(1 == distinguishable[at_i_j]) {
                    continue;
                }

                /* one state accepts and another doesn't */
                if(state_i_is_accepting
                && !set_has_elm(dfa->accepting_states, state_j)) {

                    D( printf("\t\tstate %d and state %d are distinguishable (final, non-final)\n", state_i, state_j); )

                    distinguishable[at_i_j] = 1;
                    distinguishable[at_j_i] = 1;
                    made_progress = 1;
                    continue;
                }

                /* collect ordered transition info for state j */
                collect_transitions(
                    state_j,
                    num_states,
                    largest_char,
                    dfa->state_transitions[state_j],
                    destination_states[j],
                    alphabet
                );

                /* check for distinguishability */
                for(k = 0; k <= ((unsigned) largest_char); ++k) {

                    if(1 == distinguishable[AT(
                        destination_states[i][k],
                        destination_states[j][k],
                        num_states
                    )]) {

                        D( printf("\tstate %d and state %d are distinguishable\n", state_i, state_j); )

                        distinguishable[at_i_j] = 1;
                        distinguishable[at_j_i] = 1;
                        made_progress = 1;
                        break;
                    }
                }
            }
        }
    }

    mem_free(destination_states[i]);
    mem_free(destination_states[j]);

    at_i_j = num_states + 1;
    new_state_ids = mem_calloc(at_i_j, sizeof(unsigned int));
    mdfa = nfa_alloc();

    for(k = 0; k <= num_states; ++k) {
        new_state_ids[k] = num_states;
    }

    if(LABEL_SUBSETS) {
        state_subsets = mem_calloc(at_i_j, sizeof(PSet *));
        for(state_i = 0; state_i <= num_states; ++state_i) {
            state_subsets[state_i] = set_alloc();
        }
    }

    /* create an old state id -> new state id mapping, this also adds
     * a sink state. */
    for(state_i = 0; state_i <= num_states; ++state_i) {

        if(num_states != new_state_ids[state_i]) {
            continue;
        }

        new_state_ids[state_i] = nfa_add_state(mdfa);

        D( printf("new_state_ids[%d] = %d\n", state_i, new_state_ids[state_i]); )

        if(LABEL_SUBSETS) {
            seen_chars = state_subsets[new_state_ids[state_i]];
        }

        D( printf("state[%d] = {", new_state_ids[state_i]); )
        for(state_j = 0; state_j < num_states; ++state_j) {
            if(0 == distinguishable[AT(state_i, state_j, num_states)]) {
                D( printf("%d ", state_j); )
                new_state_ids[state_j] = new_state_ids[state_i];

                /* collect the final states */
                if(set_has_elm(dfa->accepting_states, state_j)) {
                    nfa_add_accepting_state(mdfa, new_state_ids[state_i]);
                }

                if(LABEL_SUBSETS) {
                    set_add_elm(seen_chars, state_j);
                }
            }
        }

        if(LABEL_SUBSETS) {
            vector_set(
                mdfa->state_subsets,
                new_state_ids[state_i],
                seen_chars,
                delegate_do_nothing
            );

            state_subsets[new_state_ids[state_i]] = NULL;
        }

        D( printf("}\n"); )
    }

    D( printf("\n"); )

    if(LABEL_SUBSETS) {
        for(state_i = 0; state_i <= num_states; ++state_i) {
            if(NULL != state_subsets[state_i]) {
                set_free(state_subsets[state_i]);
                state_subsets[state_i] = NULL;
            }
        }
        mem_free(state_subsets);
    }

    /* go get some representatives from the dfa */
    old_state_reps = mem_calloc(mdfa->num_states, sizeof(int));
    for(k = 0; k < num_states; ++k) {
        D( printf("old_state_reps[new_state_ids[%d] = %d]\n", k, new_state_ids[k]); )
        old_state_reps[new_state_ids[k]] = k;
    }

    /* add in the transitions for all states but the sink state */
    seen_chars = set_copy(alphabet);
    mdfa_and_state_id.mdfa = mdfa;

    for(num_states = mdfa->num_states - 1, k = 0; k < num_states; ++k) {
        set_empty(seen_chars);

        for(trans = dfa->state_transitions[old_state_reps[k]];
            NULL != trans;
            trans = trans->trans_next) {

            nfa_add_value_transition(
                mdfa,
                k,
                new_state_ids[trans->to_state],
                trans->condition.value
            );

            set_add_elm(seen_chars, trans->condition.value);
        }

        mdfa_and_state_id.state = k;
        set_complement_inplace(seen_chars);
        set_intersect_inplace(seen_chars, alphabet);
        need_sink_state += set_cardinality(seen_chars);
        set_map(
            seen_chars,
            (void *) &mdfa_and_state_id,
            (PSetMapFunc *) &add_sink_state_transitions
        );
    }

    /* add in transitions on the sink state */
    if(need_sink_state) {
        mdfa_and_state_id.state = mdfa->num_states - 1;
        set_map(
            alphabet,
            (void *) &mdfa_and_state_id,
            (PSetMapFunc *) &add_sink_state_transitions
        );
    } else {
        mdfa->num_states -= 1;
    }

    set_free(alphabet);
    set_free(seen_chars);

    mem_free(old_state_reps);
    mem_free(new_state_ids);
    mem_free(distinguishable);

    return mdfa;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a new non-deterministic finite automata on the heap. This default
 * starting state is 0.
 */
PNFA *nfa_alloc(void) {

    PNFA *nfa;
    NFA_Transition **state_transitions,
                   **transition_destinations;
    NFA_TransitionGroup *group;
    PVector *state_subsets;
    int *conclusions;

    state_subsets = vector_alloc(100);

    nfa = mem_calloc(1, sizeof(PNFA));
    group = mem_calloc(1, sizeof(NFA_TransitionGroup));
    state_transitions = mem_calloc(
        NFA_NUM_DEFAULT_STATES,
        sizeof(NFA_Transition *)
    );
    transition_destinations = mem_calloc(
        NFA_NUM_DEFAULT_STATES,
        sizeof(NFA_Transition *)
    );

    conclusions = mem_calloc(NFA_NUM_DEFAULT_STATES, sizeof(int));

    if(is_null(nfa)
    || is_null(group)
    || is_null(state_transitions)
    || is_null(transition_destinations)
    || is_null(state_subsets)) {
        mem_error("Unable to allocate DFA on the heap.");
    }

    memset(conclusions, -1, NFA_NUM_DEFAULT_STATES * sizeof(int));

    nfa->state_subsets = state_subsets;
    nfa->accepting_states = set_alloc();
    nfa->current_state = 0;
    nfa->num_state_slots = NFA_NUM_DEFAULT_STATES;
    nfa->num_states = 0;
    nfa->num_transitions = 0;
    nfa->state_transitions = state_transitions;
    nfa->destination_states = transition_destinations;
    nfa->transition_group = group;
    nfa->conclusions = conclusions;
    nfa->num_unused_states = 0;

    group->next = NULL;
    group->num_transitions = 0;

    return nfa;
}

/**
 * Free a NFA.
 */
static void free_state_subset(void *subset) {
    set_free((PSet *) subset);
}
void nfa_free(PNFA *nfa) {
    int i;
    NFA_Transition *transition;
    NFA_TransitionGroup *group,
                        *next_group;

    assert_not_null(nfa);

    for(group = nfa->transition_group; is_not_null(group); group = next_group) {
        next_group = group->next;
        transition = group->transitions;
        for(i = group->num_transitions; --i >= 0; ++transition) {
            if(transition->type == T_SET) {
                set_free(transition->condition.set);
            }
        }
        mem_free(group);
    }

    vector_free(nfa->state_subsets, (PDelegate *) free_state_subset);

    set_free(nfa->accepting_states);
    mem_free(nfa->state_transitions);
    mem_free(nfa->destination_states);
    mem_free(nfa->conclusions);
    mem_free(nfa);
}

PNFA *nfa_to_dfa(PNFA *nfa, PSet *priority_set) {
    int largest_char;
    PNFA *dfa = NULL;
    assert_not_null(nfa);

    largest_char = NFA_max_alphabet_char(nfa);
    return NFA_subset_construction(nfa, priority_set, largest_char);
}

PNFA *nfa_to_mdfa(PNFA *nfa, PSet *priority_set) {
    int largest_char;
    PNFA *dfa = NULL;
    assert_not_null(nfa);

    largest_char = NFA_max_alphabet_char(nfa);
    nfa = NFA_subset_construction(nfa, priority_set, largest_char);
    dfa = DFA_minimize(nfa, largest_char);
    nfa_free(nfa);
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
         *destinations,
         *conclusions;
    unsigned int state,
                 old;

    assert_not_null(nfa);

    if(nfa->num_unused_states == 0) {

        old = nfa->num_state_slots;
        transitions = (void *) nfa->state_transitions;
        destinations = (void *) nfa->destination_states;
        conclusions = (void *) nfa->conclusions;

        NFA_alloc_slot(
            &destinations,
            &nfa->num_states,
            &nfa->num_state_slots,
            sizeof(NFA_Transition *),
            0
        );
        NFA_alloc_slot(
            &conclusions,
            &nfa->num_states,
            &nfa->num_state_slots,
            sizeof(int),
            0
        );
        NFA_alloc_slot(
            &transitions,
            &nfa->num_states,
            &nfa->num_state_slots,
            sizeof(NFA_Transition *),
            1
        );

        if(nfa->num_state_slots > old) {
            memset(
                nfa->destination_states + old,
                0,
                sizeof(NFA_Transition *) * (nfa->num_state_slots - old)
            );
            memset(
                nfa->state_transitions + old,
                0,
                sizeof(NFA_Transition *) * (nfa->num_state_slots - old)
            );
            memset(
                nfa->conclusions + old,
                -1,
                sizeof(int) * (nfa->num_state_slots - old)
            );
        }

        state = nfa->num_states - 1;
    } else {
        state = nfa->unused_states[--(nfa->num_unused_states)];
    }

    nfa->conclusions[state] = -1;
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
 * Add a 'conclusion' to particular accepting state. I.e. if being at some
 * accepting state implies something then this is a means to add that
 * information so that such an implication can be checked later.
 */
void nfa_add_conclusion(PNFA *nfa, unsigned int which_state, int conclusion) {
    int *c;
    assert_not_null(nfa);
    if(set_has_elm(nfa->accepting_states, which_state)) {
        c = nfa->conclusions + which_state;
        *c = conclusion;
    }
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
    NFA_mark_unused_state(nfa, state_b);
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
                              int test_value) {

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

/* -------------------------------------------------------------------------- */

typedef struct nfa_state_pair {
    unsigned int from_state;
    unsigned int to_state;
} NFAStatePair;

static uint32_t nfa_state_pair_hash(void *pair) {
    uint64_t together;
    union {
        uint32_t uint32;
        uint64_t uint64;
        void *pointer;
    } as;

    NFAStatePair *state_pair;

    /* init */
    state_pair = (struct nfa_state_pair *) pair;
    as.pointer = 0;
    as.uint64 = 0;

    switch(sizeof(void *)) {
    case 8:
        together = (state_pair->from_state * 33) ^ state_pair->to_state;
        together <<= 32;
        together |= state_pair->from_state ^ (state_pair->to_state * 57);
        as.uint64 = together;
        break;
    case 4:
        as.uint32 = (
            (state_pair->from_state * 33) ^ (state_pair->to_state * 57)
        );
        break;
    }

    return dict_pointer_hash_fnc(as.pointer);
}

static int nfa_state_pair_collision(void *a, void *b) {
    struct nfa_state_pair *pair_a = (struct nfa_state_pair *) a;
    struct nfa_state_pair *pair_b = (struct nfa_state_pair *) b;

    return pair_a->from_state != pair_b->from_state
        || pair_a->to_state != pair_b->to_state;
}

static void print_state_id(int *seen, unsigned state_id) {
    if(*seen) {
        printf(", ");
    }
    if(0 == ++*seen % 5) {
        printf("<BR />");
    }
    printf("%d", state_id);
}

static void print_state_subset(PNFA *nfa, unsigned state) {
    int seen = 0;
    PSet *state_ids;

    if(!LABEL_SUBSETS) {
        return;
    }

    state_ids = vector_get(nfa->state_subsets, state);

    if(NULL == state_ids) {
        return;
    }

    printf(" <BR /> {");
    set_map(state_ids, &seen, (PSetMapFunc *) &print_state_id);
    printf("}");
}

/**
 * Reverse the arrows in a NFA in-place.
 */
static void NFA_reverse(PNFA *nfa) {
    NFA_Transition *trans;
    unsigned state;
    unsigned temp;

    for(state = 0; state < nfa->num_states; ++state) {
        trans = nfa->state_transitions[state];
        if(NFA_UNUSED_STATE == trans) {
            continue;
        }

        for(; is_not_null(trans); trans = trans->trans_next) {
            temp = trans->from_state;
            trans->from_state = trans->to_state;
            trans->to_state = temp;
        }
    }
}

/**
 * Update a set of reachable states given a start state.
 */
static void NFA_get_reachable_from(PNFA *nfa, PSet *reached, unsigned initial) {
    NFA_Transition *trans;
    unsigned state;
    unsigned found = 0;

    set_add_elm(reached, initial);

    do {
        found = 0;
        for(state = 0; state < nfa->num_states; ++state) {
            for(trans = nfa->state_transitions[state];
                is_not_null(trans);
                trans = trans->trans_next) {

                if(set_has_elm(reached, trans->from_state)
                && !set_has_elm(reached, trans->to_state)) {
                    set_add_elm(reached, trans->to_state);
                    found = 1;
                }
            }
        }
    } while(found);
}

/**
 * Get the set of reachable states and return it.
 */
static PSet *NFA_get_reachable_states(PNFA *nfa) {
    PSet *forward = set_alloc();
    PSet *backward = set_alloc();
    unsigned state;

    /* scale the sets */
    set_add_elm(forward, nfa->num_states);
    set_remove_elm(forward, nfa->num_states);
    set_add_elm(backward, nfa->num_states);
    set_remove_elm(backward, nfa->num_states);

    /* find reachable from the start state */
    NFA_get_reachable_from(nfa, forward, nfa->start_state);

    /* find the states that reach the final states */
    NFA_reverse(nfa);
    for(state = 0; state < nfa->num_states; ++state) {
        if(set_has_elm(nfa->accepting_states, state)
        && !set_has_elm(backward, state)) {
            NFA_get_reachable_from(nfa, backward, state);
        }
    }
    NFA_reverse(nfa);

    /* keep the intersection */
    set_intersect_inplace(forward, backward);
    set_free(backward);

    return forward;
}

/**
 * Print out the NFA in the DOT language.
 */
void nfa_print_dot(PNFA *nfa) {

    unsigned int i = nfa->num_states;
    unsigned int j;
    unsigned int state = 0;

    /* for keeping track of transition id pairs */
    NFAStatePair *trans_ids;
    NFAStatePair *trans_id;

    NFA_Transition *transition;
    NFA_Transition **state_transitions = (
        (NFA_Transition **) nfa->state_transitions
    );

    const unsigned num_slots = nfa->num_transitions / (nfa->num_states + 1);
    char *transition_chars = 0;
    char *transition_char;
    const char *sep = ", ";
    const char *sep_offset;

    PDictionary *trans_set = 0;
    PDictionaryGenerator *ids = 0;

    printf("digraph {\n");

    trans_ids = (NFAStatePair *) mem_calloc(
        (1 + nfa->num_transitions), sizeof(NFAStatePair)
    );
    trans_id = &(trans_ids[0]);

    for(state = 0; i--; ++state) {

        transition = state_transitions[state];

        if(transition == NFA_UNUSED_STATE) {
            continue;
        }

        if(set_has_elm(nfa->accepting_states, state)) {
            printf(
                "x%d [label=<<FONT POINT-SIZE=\"20\">%d</FONT>",
                state,
                state
            );

            print_state_subset(nfa, state);

            printf("> shape=doublecircle] \n");
        } else {
            printf(
                "x%d [label=< <FONT POINT-SIZE=\"20\">%d</FONT>",
                state,
                state
            );

            print_state_subset(nfa, state);

            printf("> shape=circle] \n");
        }

        if(state == nfa->start_state) {
            printf("xSTART [color=white label=\"\"] \n");
            printf("xSTART -> x%d \n", state);
        }

        trans_set = dict_alloc(
            num_slots,
            nfa_state_pair_hash,
            nfa_state_pair_collision
        );

        for(/* */;
            is_not_null(transition);
            transition = transition->trans_next, ++trans_id) {

            trans_id->from_state = transition->from_state;
            trans_id->to_state = transition->to_state;

            switch(transition->type) {
                case T_VALUE:
                    if(!dict_is_set(trans_set, trans_id)) {

                        transition_chars = calloc(256UL, sizeof(char));
                        dict_set(
                            trans_set,
                            trans_id,
                            transition_chars,
                            delegate_do_nothing
                        );
                    }

                    transition_chars = dict_get(trans_set, trans_id);
                    transition_chars[
                        (size_t) ((unsigned char) transition->condition.value)
                    ] = (char) transition->condition.value;

                    break;
                case T_SET:
                    printf(
                        "x%d -> x%d [label=< {...} >] \n",
                        state,
                        transition->to_state
                    );
                    break;
                case T_EPSILON:
                    printf(
                        "x%d -> x%d [label=< >] \n", /* &#949; */
                        state,
                        transition->to_state
                    );
                    break;
                default:
                    break;
            }
        }

        ids = dict_keys_generator_alloc(trans_set);
        for(; generator_next(ids); ) {

            trans_id = generator_current(ids);
            transition_chars = dict_get(trans_set, trans_id);

            printf(
                "x%d -> x%d [label=<<FONT face=\"Courier\"> ",
                trans_id->from_state,
                trans_id->to_state
            );

            for(sep_offset = &(sep[1]), j = 0,
                transition_char = &(transition_chars[256]);
                transition_char-- > &(transition_chars[0]); ) {

                if(0 == *transition_char) {
                    continue;
                }

                if(j > 0 && 0 == j % 5) {
                    printf(" <BR />");
                    sep_offset = &(sep[1]);
                }

                if(isgraph(*transition_char)
                && !isspace(*transition_char)) {
                    if('<' == *transition_char) {
                        printf("%s&lt;", sep_offset);
                    } else if('>' == *transition_char) {
                        printf("%s&gt;", sep_offset);
                    } else {
                        printf("%s%c", sep_offset, *transition_char);
                    }
                } else {
                    printf("%s0x%x", sep_offset, (int) *transition_char);
                }

                sep_offset = &(sep[0]);
                ++j;
            }

            printf(" </FONT>>] \n");
        }

        generator_free(ids);
        dict_free(
            trans_set,
            delegate_do_nothing,
            free
        );

        ids = 0;
        trans_set = 0;
    }

    mem_free(trans_ids);
    trans_ids = 0;

    printf("}\n");
}

/**
 * Print out the NFA as a right-linear grammar formatted in LaTeX. This does
 * not output productions for unreachable or sink states.
 */
void nfa_print_latex(PNFA *nfa) {

    unsigned int i = nfa->num_states;
    unsigned int j;
    unsigned int state = 0;

    NFA_Transition *transition;
    NFA_Transition **state_transitions = (
        (NFA_Transition **) nfa->state_transitions
    );

    PSet *reachable = NFA_get_reachable_states(nfa);

    const unsigned num_slots = nfa->num_transitions / (nfa->num_states + 1);
    const char *sep = "|\\ ";
    const char *sep_offset;

    assert_not_null(nfa);

    printf("\\begin{eqnarray*}\n");

    for(state = 0; i--; ++state) {

        transition = state_transitions[state];

        if(transition == NFA_UNUSED_STATE) {
            continue;
        } else if(!set_has_elm(reachable, state)) {
            continue;
        }

        sep_offset = &(sep[2]);

        if(set_has_elm(nfa->accepting_states, state)) {
            printf("S_{%d} & \\to & \\epsilon\\ ", state);
            sep_offset = sep;

        } else {
            printf("S_{%d} & \\to & ", state);
        }

        for(j = 0;
            is_not_null(transition);
            transition = transition->trans_next) {

            if(!set_has_elm(reachable, transition->to_state)) {
                continue;
            }

            if(j > 0 && 0 == (j % 10)) {
                printf("\\\\\n & \\to & ");
                sep_offset = &(sep[2]);
            }

            ++j;

            switch(transition->type) {
                case T_VALUE:
                    printf(
                        "%s \\texttt{%c}\\ S_{%d}\\ ",
                        sep_offset,
                        (char) transition->condition.value,
                        transition->to_state
                    );

                    break;
                case T_EPSILON:
                    printf(
                        "%s\\ S_{%d}\\ ",
                        sep_offset,
                        transition->to_state
                    );
                    break;
                default:
                    break;
            }

            sep_offset = sep;
        }

        printf("\\\\ \n");
    }

    printf("\\end{eqnarray*}\n");

    set_free(reachable);
}

/* -------------------------------------------------------------------------- */

#define P fprintf

/**
 * Print a NFA as a C scanner into the file specified.
 */
void nfa_print_scanner(const PNFA *nfa,
                       const char *out_file,
                       const char *func_name) {
    FILE *F;
    PSet *astates;
    NFA_Transition *trans,
                   **transitions;
    int i,
        n;

    assert_not_null(nfa);
    assert_not_null(out_file);

    F = fopen(out_file, "w");
    if(is_null(F)) {
        std_error("Error: Unable to create file for NFA export");
    }

    astates = nfa->accepting_states;
    transitions = nfa->state_transitions;

    P(F, "\n");
    P(F, "#ifndef _P_SCANNER_%s_\n", func_name);
    P(F, "#define _P_SCANNER_%s_\n", func_name);
    P(F, "#include <ctype.h>\n");
    P(F, "#include <std-include.h>\n");
    P(F, "#include <p-types.h>\n");
    P(F, "#include <p-scanner.h>\n\n");
    P(F, "extern G_Terminal %s(PScanner *);\n\n", func_name);
    P(F, "G_Terminal %s(PScanner *S) {\n", func_name);
    P(F, "    G_Terminal term = -1;\n");
    P(F, "    unsigned int seen_accepting_state = 0;\n");
    P(F, "    int cc, nc = 0, pnc = 0;\n");
    P(F, "    scanner_skip(S, &isspace);\n");
    P(F, "    scanner_mark_lexeme_start(S);\n");

    if(nfa->start_state > 0) {
        P(F, "    goto state_%d;\n", nfa->start_state);
    }

    for(n = 0, i = nfa->num_states; --i >= 0; ++n) {

        P(F, "state_%d:\n", n);

        trans = transitions[n];
        if(is_null(trans) && set_has_elm(astates, n)) {
            P(F, "    term = %d;\n", *(nfa->conclusions+n));
            P(F, "    goto commit;\n");
        } else {
            if(set_has_elm(astates, n)) {
                P(F, "    term = %d;\n", *(nfa->conclusions+n));
                P(F, "    pnc = nc;\n");
                P(F, "    seen_accepting_state = 1;\n");
            }
            P(F, "    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }\n");
            P(F, "    ++nc;\n");
            P(F, "    switch(cc) {\n");
            for(; is_not_null(trans); trans = trans->trans_next) {

                if(trans->type != T_VALUE) {
                    std_error(
                        "Internal NFA Print Error: Cannot print non-value "
                        "transitions."
                    );
                }

                P(
                    F,
                    "        case %d: goto state_%d;\n",
                    trans->condition.value,
                    trans->to_state
                );
            }
            P(F, "        default: goto undo_and_commit;\n");
            P(F, "    }\n");
        }
    }

    P(F, "undo_and_commit:\n");
    P(F, "    if(!seen_accepting_state) {\n");
    P(F, "        return -1;\n");
    P(F, "    }\n");
    P(F, "    scanner_pushback(S, nc - pnc);\n");
    P(F, "commit:\n");
    P(F, "    scanner_mark_lexeme_end(S);\n");
    P(F, "    return term;\n");
    P(F, "}\n\n");
    P(F, "#endif\n\n");

    fclose(F);

    return;
}
