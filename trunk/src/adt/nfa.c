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
    NFA_Transition *trans,
                   *prev;
    NFA_TransitionGroup *group;

    assert_not_null(nfa);
    assert(start_state < nfa->num_states);
    assert(end_state < nfa->num_states);

    if(nfa->transition_group->num_transitions >= NFA_NUM_DEFAULT_TRANSITIONS) {
        group = mem_alloc(sizeof(NFA_TransitionGroup));
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
                                  int transition_type) {
    NFA_StateStack stack;
    NFA_Transition *transition;

    int accepting_id = -1,
        i;

    stack.ptr = stack.bottom;
    stack.top = stack.bottom + NFA_MAX_EPSILON_STACK;

    if(is_null(states) || !set_cardinality(states)) {
        return 0;
    }

    D( printf("\t finding epsilon closure... \n"); )

    /* add the states in the set to the stack */
    set_map(states, (void *) &stack, (PSetMapFunc *) &NFA_state_stack_push);

    while(stack.ptr > stack.bottom && stack.ptr < stack.top) {

        transition = nfa->state_transitions[*(--stack.ptr)];

        if(set_has_elm(nfa->accepting_states, *stack.ptr)) {
            D( printf("\t\t\t %d is an accepting state. \n", *stack.ptr); )
            accepting_id = *stack.ptr;
        }

        for(; is_not_null(transition); transition = transition->trans_next) {
            if(set_has_elm(states, transition->to_state)) {
                continue;
            }
            if(transition->type == T_EPSILON) {
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

    const int num_transitions = nfa->num_transitions;

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

/* DFA state, represents a subset of all NFA states. */
typedef struct DFA_State {
    PSet *nfa_states;
    unsigned int id,
                 accepting_id;
} DFA_State;

/**
 * Perform the subset construction on the NFA to turn it into a DFA.
 */
static PNFA *NFA_subset_construction(PNFA *nfa, int largest_char) {

    unsigned int next_state_id,
                 prev_state_id,
                 num_dfa_states = 0;

    int c,
        as,
        is_accepting;

    DFA_State dfa_state_stack[NFA_NUM_DEFAULT_STATES],
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

    as = NFA_transitive_closure(nfa, dfa_state_stack->nfa_states, T_EPSILON);
    if(as >= 0) {
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
            as = NFA_transitive_closure(nfa, transition_set, T_EPSILON);

            /* we have already seen this DFA state */
            next_state_id = (unsigned int) dict_get(dfa_states, transition_set);

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
                    (void *) (next_state_id + 1),
                    &delegate_do_nothing
                );

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
 * Minimize a DFA. The main idea is that we want to find the partition the set
 * of DFA states into equivalence classes, and then each equivalence class
 * represents a minimized DFA state.
 *
 * The way we generate the classes is by repeatedly marking pairs of states that
 * we know to be distinguishable until no more marking is done. Once we have
 * done this, it is merely about extracting the classes from the marking table,
 * building the minimized DFA states (easy), and then figuring out the new
 * transitions (slightly more involved).
 *
 * Note: This procedure assumes that all transitions are value transitions.
 *
 * Note: Two accepting states will be seen as distinguishable if they both have
 *       different conclusions associated with them.
 */
static PNFA *DFA_minimize(PNFA *dfa, int largest_char) {

    PNFA *min_dfa;

    PSet * const astates = dfa->accepting_states,
         *seen,
         **mdfa_states;

    const int num_states = dfa->num_states,
              num_transitions = dfa->num_transitions,
              ss = (int) dfa->start_state;

    char *mark_table,
         *row,
         *cell, /* refers to some (i, j) location in the marking matrix */
         *cell_p, /* refers to the (j, i) locating in the marking matrix */
         *temp;

    NFA_Transition *trans,
                   ** const source_transitions = dfa->state_transitions;

    NFA_TransitionGroup *groups = dfa->transition_group,
                        *group;

    int i = dfa->num_transitions,
        j, k, m, n, o,
        *state_trans_values,
        *states_to_mstates;

    const long int jp = num_states - 1,
                   jjp = num_states * jp;

    /* ------------------ */

    D( printf("configuring global minimize.. \n"); )

    mark_table = mem_alloc((sizeof(char) * num_states) * num_states);
    if(is_null(mark_table)) {
        mem_error("Internal DFA Error: Cannot create marking table.");
    }

    D( printf("marking initial distinguishable transitions... \n"); )

    /* populate the table and mark all transitions where one of the states is
     * accepting and the other is not. */
    cell = mark_table;
    states_to_mstates = dfa->conclusions;
    for(i = num_states; --i >= 0; ) {
        m = set_has_elm(astates, i);
        for(j = num_states; --j >= 0; ++cell) {
            n = set_has_elm(astates, j);
            if(m && n) {
                if(states_to_mstates[i] != states_to_mstates[j]) {
                    *cell = 1;
                    continue;
                }
            }
            *cell = (m != n);
        }
    }

    D( printf("running main algorithm... \n"); )

    state_trans_values = mem_alloc(sizeof(int) * (++largest_char));
    if(is_null(mark_table)) {
        mem_error("Internal DFA Error: Cannot create value transition table.");
    }
    memset(state_trans_values, -1, sizeof(int) * largest_char);

    do {
        D( printf("marking cells... \n"); )

        m = 0;
        row = mark_table;
        cell = mark_table;

        for(i = num_states; --i >= 0; row += num_states) {

            /* fill in the values, this will allow us to compare transitions between
             * any two states. */
            trans = source_transitions[i];
            for(; is_not_null(trans); trans = trans->trans_next) {
                state_trans_values[trans->condition.value] = trans->to_state;
            }

            D( printf("Looking at state %d \n", i); )

            for(group = groups; is_not_null(group); group = group->next) {
                trans = group->transitions;
                for(o = group->num_transitions; --o >= 0; ++trans) {

                    cell = row + (num_states - trans->from_state - 1);
                    cell_p = mark_table
                           + (jjp - (num_states * trans->from_state) + jp - i);

                    if(*cell || *cell_p) {
                        continue;
                    }

                    k = state_trans_values[trans->condition.value];
                    if(k < 0) {
                        D( printf("\t marking (%d, %d) \n", i, trans->from_state); )
                        *cell = 1;
                        *cell_p = 1;
                        ++m;
                        continue;
                    }

                    temp = mark_table + (jjp - (num_states * k) + jp - trans->to_state);
                    if(*temp) {
                        ++m;
                        *cell = 1;
                        *cell_p = 1;
                        D( printf("\t marking (%d, %d) \n", i, trans->from_state); )
                    }
                }
            }

            /* undo the changes made to the state_trans_values table */
            trans = source_transitions[i];
            for(; is_not_null(trans); trans = trans->trans_next) {
                state_trans_values[trans->condition.value] = -1;
            }
        }
    } while(m > 0);

    D( printf("states marked. \n"); )

    /* go and build the minimized DFA */
    states_to_mstates = mem_alloc(sizeof(int) * num_states);
    if(is_null(states_to_mstates)) {
        mem_error("Internal DFA Error: Unable to complete DFA minimization.");
    }

    D( printf("making minimized DFA states. \n"); )

    min_dfa = nfa_alloc();
    seen = set_alloc();
    row = mark_table + jjp + num_states - 1;
    k = num_states;
    /* start by creating all minimized DFA states and a mapping between DFA
     * states and minimized-DFA states. This works by starting at the bottom-
     * right of the mark_table and ascending up the main diagonal. */
    for(i = num_states; --i >= 0; --k, row -= num_states) {
        if(set_has_elm(seen, i)) {
            continue;
        }

        m = nfa_add_state(min_dfa);
        set_add_elm(seen, i);

        cell = row - (num_states - k);

        D( printf("\t added mstate %d. \n", m); )

        for(n = num_states - k, j = k; --j >= 0; ++n, --cell) {
            if(*cell) {
                continue;
            }

            D( printf("\t\t has state %d \n", n); )

            states_to_mstates[n] = m;
            set_add_elm(seen, j);

            if(ss == n) {
                nfa_change_start_state(min_dfa, m);
            }

            if(set_has_elm(astates, n)) {
                nfa_add_accepting_state(min_dfa, m);
                nfa_add_conclusion(
                    min_dfa,
                    m,
                    *(dfa->conclusions + n)
                );
            }
        }
    }

    D( printf("constructed minimized DFA states. \n"); )

    mem_free(mark_table);
    mem_free(state_trans_values);
    set_free(seen);

    /* weed to keep track of what *values* we've added transitions for on the
     * particular minimized DFA states. */
    mdfa_states = mem_alloc(sizeof(PSet *) * min_dfa->num_states);
    if(is_null(mdfa_states)) {
        mem_error("Internal DFA Error: Unable to finish off DFA minimization.");
    }
    n = (int) min_dfa->num_states;
    for(i = 0; i < n; ++i) {
        mdfa_states[i] = set_alloc();
    }

    D( printf("adding minimized transitions. \n"); )

    /* go over all of the transitions and add in the min_dfa transitions where
     * needed. */
    for(group = groups; is_not_null(group); group = group->next) {
        trans = group->transitions;
        for(o = group->num_transitions; --o >= 0; ++trans) {
            j = states_to_mstates[trans->from_state];
            if(set_has_elm(mdfa_states[j], trans->condition.value)) {
                continue;
            }

            D(
               printf(
                   "\t adding dfa transition %d -> %d to min-dfa as %d -> %d \n",
                   trans->from_state,
                   trans->to_state,
                   j,
                   states_to_mstates[trans->to_state]
               );
            )

            set_add_elm(mdfa_states[j], trans->condition.value);

            nfa_add_value_transition(
                 min_dfa,
                 j,
                 states_to_mstates[trans->to_state],
                 trans->condition.value
            );

        }
    }

    /* clean up */
    for(i = 0; i < n; ++i) {
        set_free(mdfa_states[i]);
    }

    mem_free(mdfa_states);
    mem_free(states_to_mstates);

    D( printf("done. \n"); )

    return min_dfa;
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
    int *conclusions;

    nfa = mem_alloc(sizeof(PNFA));
    group = mem_alloc(sizeof(NFA_TransitionGroup));
    state_transitions = mem_calloc(
        NFA_NUM_DEFAULT_STATES,
        sizeof(NFA_Transition *)
    );
    transition_destinations = mem_calloc(
        NFA_NUM_DEFAULT_STATES,
        sizeof(NFA_Transition *)
    );

    conclusions = mem_alloc(NFA_NUM_DEFAULT_STATES * sizeof(int));

    if(is_null(nfa)
    || is_null(group)
    || is_null(state_transitions)
    || is_null(transition_destinations)) {
        mem_error("Unable to allocate DFA on the heap.");
    }

    memset(conclusions, -1, NFA_NUM_DEFAULT_STATES * sizeof(int));

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

    set_free(nfa->accepting_states);
    mem_free(nfa->state_transitions);
    mem_free(nfa->destination_states);
    mem_free(nfa->conclusions);
    mem_free(nfa);
}

PNFA *nfa_to_dfa(PNFA *nfa) {
    int largest_char, i;
    PNFA *dfa = NULL;
    NFA_Transition **transitions,
                   *trans;
    assert_not_null(nfa);

    largest_char = NFA_max_alphabet_char(nfa);
    nfa = NFA_subset_construction(nfa, largest_char);
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

        transition = state_transitions[state];

        if(transition == NFA_UNUSED_STATE) {
            continue;
        }

        if(set_has_elm(nfa->accepting_states, state)) {
            printf(
                "Ox%d [label=<S<FONT POINT-SIZE=\"8\"> %d</FONT>(%d)> "
                "shape=doublecircle] \n",
                state,
                state,
                nfa->conclusions[state]
            );
        } else {
            printf(
                "Ox%d [label=<S<FONT POINT-SIZE=\"8\"> %d</FONT>>] \n",
                state,
                state
            );
        }

        if(state == nfa->start_state) {
            printf("Ox%d [color=blue] \n", state);
        }

        for(; is_not_null(transition); transition = transition->trans_next) {
            switch(transition->type) {
                case T_VALUE:
                    if(transition->condition.value < 32
                    || transition->condition.value > 126
                    || transition->condition.value == 60
                    || transition->condition.value == 62) {
                        printf(
                            "Ox%d -> Ox%d [label=< '#%d'<FONT POINT-SIZE=\"8\"> %d</FONT> >] \n",
                            state,
                            transition->to_state,
                            transition->condition.value,
                            transition->id
                        );
                    } else {
                        printf(
                            "Ox%d -> Ox%d [label=< '%c'<FONT POINT-SIZE=\"8\"> %d</FONT> >] \n",
                            state,
                            transition->to_state,
                            (unsigned char) transition->condition.value,
                            transition->id
                        );
                    }
                    break;
                case T_SET:
                    printf(
                        "Ox%d -> Ox%d [label=< {...}<FONT POINT-SIZE=\"8\"> %d</FONT> >] \n",
                        state,
                        transition->to_state,
                        transition->id
                    );
                    break;
                case T_EPSILON:
                    printf(
                        "Ox%d -> Ox%d [label=< &#949;<FONT POINT-SIZE=\"8\"> %d</FONT> >] \n",
                        state,
                        transition->to_state,
                        transition->id
                    );
                    break;
                default:
                    break;
            }
        }
    }
}

/* -------------------------------------------------------------------------- */

#define P fprintf

/**
 * Print a NFA as C code into the file specified.
 */
void nfa_print_to_file(const PNFA *nfa, const char *out_file) {
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
    transitions = (NFA_Transition **) nfa->state_transitions;

    P(F, "\n");
    P(F, "#include <std-include.h>\n");
    P(F, "#include <p-types.h>\n");
    P(F, "#include <p-scanner.h>\n\n");
    P(F, "extern G_Terminal lex_analyze(PScanner *);\n\n");
    P(F, "G_Terminal lex_analyze(PScanner *S) {\n");
    P(F, "    G_Terminal pterm = -1, term = -1;\n");
    P(F, "    unsigned int n = 0, seen_accepting_state = 0;\n");
    P(F, "    int cc, nc = 0, pnc = 0;\n");
    P(F, "    scanner_mark_lexeme_start(S);\n");

    if(nfa->start_state > 0) {
        P(F, "    goto state_%d;\n", nfa->start_state);
    }

    for(n = 0, i = nfa->num_states; --i >= 0; ++n) {

        P(F, "state_%d:\n", n);
        P(F, "    if(!(cc = scanner_advance(S))) { goto undo_and_commit; }\n");

        if(set_has_elm(astates, n)) {
            P(F, "    pterm = term;\n");
            P(F, "    term = %d;\n", *(nfa->conclusions+n));
            P(F, "    pnc = nc;\n");
            P(F, "    seen_accepting_state = 1;\n");
        }

        P(F, "    ++nc;\n");
        P(F, "    switch(cc) {\n");

        trans = transitions[n];
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

    P(F, "undo_and_commit:\n");
    P(F, "    if(!seen_accepting_state) {\n");
    P(F, "        std_error(\"Scanner Error: Unable to match token.\");\n");
    P(F, "    }\n");
    P(F, "    scanner_pushback(S, nc - pnc);\n");
    P(F, "    scanner_mark_lexeme_end(S);\n");
    P(F, "    return pterm;\n");
    P(F, "}\n\n");

    fclose(F);

    return;
}
