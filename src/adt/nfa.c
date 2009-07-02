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

#define D(x) x

#define NFA_NUM_DEFAULT_STATES 256
#define NFA_NUM_DEFAULT_TRANSITIONS 256
#define NFA_NUM_DEFAULT_STATE_TRANSITIONS 4
#define NFA_MAX_EPSILON_STACK 256
#define NFA_UNUSED_STATE ((void *) 0x1)

typedef struct NFA_Transition {

    enum {
        T_VALUE,
        T_SET,
        T_EPSILON,
        T_UNUSED
    } type;

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
    trans->from_state = start_state;
    trans->to_state = end_state;
    trans->trans_next = NULL;
    trans->dest_next = NULL;
    trans->id = nfa->num_transitions - 1;

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
 * Returns if the DFA state represented by the e-closure has an NFA state that
 * is accepting.
 */
static int NFA_transitive_closure(PNFA *nfa,
                                  PSet *states,
                                  int transition_type) {
    PSet *transitions;
    NFA_StateStack stack;
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
    set_map(states, (void *) &stack, (PSetMapFunc *) &NFA_state_stack_push);
    set_truncate(states);
    transitions = states; /* more meaningful */

    while(stack.ptr > stack.bottom && stack.ptr < stack.top) {

        transition = nfa->state_transitions[*(--stack.ptr)];

        if(set_has_elm(nfa->accepting_states, *stack.ptr)) {
            D( printf("\t\t\t %d is an accepting state. \n", *stack.ptr); )
            is_accepting = 1;
        }

        for(; is_not_null(transition); transition = transition->trans_next) {

            if(set_has_elm(transitions, transition->id)) {
                continue;
            }

            if(transition->type == T_EPSILON) {
                NFA_state_stack_push(&stack, transition->to_state);
            } else {
                set_add_elm(transitions, transition->id);
            }
        }
    }

    D( printf("\t done. \n"); )

    return is_accepting;
}

/**
 * Simulate transitions from the transitions in input_transitions to their
 * respective states and then return that set of states. *
 */
static PSet *NFA_simulate_transition(PNFA *nfa,
                                     PSet *input_transitions,
                                     int input) {

    int i = nfa->num_transitions;
    PSet *output_states = NULL;
    NFA_Transition *transition = (NFA_Transition *) nfa->transitions;

    D( printf("\t simulating transition on ASCII %d... \n", input); )

    for(; --i >= 0; ++transition) {

        if(!set_has_elm(input_transitions, transition->id)) {
            continue;
        }

        switch(transition->type) {
        case T_VALUE:
            if(transition->condition.value == input) {

                D( printf(
                    "\t\t transition found on '%c', adding state %d \n",
                    (char) input,
                    transition->to_state
                ); )

                if(is_null(output_states)) {
                    output_states = set_alloc();
                }

                set_add_elm(output_states, transition->to_state);
            }
            break;
        case T_SET:
            if(set_has_elm(transition->condition.set, input)) {

                D( printf(
                    "\t\t transition found on '%c', adding state %d \n",
                    (char) input,
                    transition->to_state
                ); )

                if(is_null(output_states)) {
                    output_states = set_alloc();
                }

                set_add_elm(output_states, transition->to_state);
            }
            break;
        default:
            break;
        }
    }

    D( printf("\t done. \n"); )

    return output_states;
}

/**
 * Determine the largest character of the alphabet that this dfa recognizes.
 */
static unsigned int NFA_max_alphabet_char(PNFA *nfa) {

    unsigned int i = nfa->num_transitions;
    int max = 0,
        j;

    NFA_Transition *trans = (NFA_Transition *) nfa->transitions;

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

/**
 * Perform the subset construction on the NFA to turn it into a DFA.
 */
typedef struct DFA_State {
    PSet *nfa_states;
    unsigned int id;
} DFA_State;

static PNFA *NFA_subset_construction(PNFA *nfa, int largest_char) {

    unsigned int next_state_id,
                 prev_state_id,
                 num_dfa_states = 0;

    int c,
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

    if(NFA_transitive_closure(nfa, dfa_state_stack->nfa_states, T_EPSILON)) {
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
            is_accepting = 0;
            if(NFA_transitive_closure(nfa, transition_set, T_EPSILON)) {
                is_accepting = 1;
            }

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
            if(is_accepting) {
                nfa_add_accepting_state(dfa, next_state_id);
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
typedef struct {
    PSet *partitions[NFA_NUM_DEFAULT_STATES],
         **top_partition,
         **max_patition,
         **curr_partition,
         *new_partition;
    PNFA *dfa;
    int largest_char;
} DFA_Partition;

/**
 * Perform localized minimization on a NFA. For simple NFAs this will generally
 * work at minimization; however, for more complex ones this will simply not do
 * anything as it only looks at the local similarities between states instead of
 * the global picture.
 *
 * The local minimization algorithm works by growing outward to other states,
 * marking and merging states as it goes.
 */
static void DFA_local_minimize(PNFA *dfa, int largest_char) {

    PSet *astates = dfa->accepting_states,
         *seen_set = set_alloc(),
         *merged_set = set_alloc(),
         **trans_to_states,
         *equiv_trans_set;

    NFA_StateStack stack,
                   test_stack;

    NFA_Transition *trans,
                   *trans_next,
                   *all_transitions = (NFA_Transition *) dfa->transitions,
                   **all_destinations = (NFA_Transition **) dfa->destination_states,
                   **all_sources = (NFA_Transition **) dfa->state_transitions;

    unsigned int state,
                 state_a,
                 state_b,
                 *stack_top,
                 *test_stack_next;

    int i, /* counter used here and there */
        j, /* " */
        k, /* " */
        *state_trans_values,
        malloced_trans_to_states = 0;

    size_t trans_table_size = sizeof(PSet *) * ++largest_char,
           state_trans_table_size = sizeof(int) * dfa->num_states;

    D( printf("\n\n"); )
    D( printf("configuring DFA minimizer. \n"); )

    /* initialize the two stacks */
    stack.ptr = stack.bottom;
    stack.top = stack.bottom + NFA_MAX_EPSILON_STACK;

    /* an array to keep track of if we've already seen a particular transition
     * for a given value going into the current state. */
    trans_to_states = alloca(trans_table_size);
    if(is_null(trans_to_states)) {
        trans_to_states = mem_alloc(trans_table_size);
        malloced_trans_to_states = 1;
        if(is_null(trans_to_states)) {
            mem_error("Internal DFA Error: Unable to minimize DFA.");
        }
    }

    /* an array to keep track of a single value of a transition at a time. */
    state_trans_values = mem_alloc(state_trans_table_size);
    if(is_null(state_trans_values)) {
        mem_error(
            "Internal DFA Error: Unable to keep track of states for DFA "
            "minimization."
        );
    }

    memset(state_trans_values, -1, state_trans_table_size);
    memset(trans_to_states, 0, trans_table_size);

    set_map(astates, (void *) &stack, (PSetMapFunc *) &NFA_state_stack_push);

    D( printf("starting.. \n"); )

    while(stack.ptr > stack.bottom && stack.ptr < stack.top) {

        state = *(--stack.ptr);

        D( printf("\t looking for states to group at state %d \n", state); )

        /* we've already seen and merged this state */
        if(set_has_elm(merged_set, state)) {
            continue;
        }

        set_add_elm(seen_set, state);

        if(is_null(all_destinations[state])) {
            continue;
        }

        D( printf("\t grouping states with similar in-bound transitions. \n"); )

        /* go look for transitions that enter the current state on the same
         * condition value. */
        for(trans = all_destinations[state];
            is_not_null(trans);
            trans = trans->dest_next) {

            /* we've already processed this state and all of its out-bound
             * transitions. We need to add a special condition to not ignore
             * self-transitions. */
            if(set_has_elm(seen_set, trans->from_state)
            && trans->from_state != state) {
                continue;
            }

            if(is_null(trans_to_states[trans->condition.value])) {
                trans_to_states[trans->condition.value] = set_alloc();
            }

            /* build up sets of states entering into the current state that all
             * transition in on the same value. */
            D(
              printf(
                  "\t\t Adding transition %d on char '%c' to group. \n",
                  trans->id,
                  (char) trans->condition.value
              );
            )

            set_add_elm(
                trans_to_states[trans->condition.value],
                trans->id
            );
        }

        D( printf("\t done grouping. \n"); )
        D( printf("\t testing groups. \n"); )

        /* go look for sets of states who have transitions into the current
         * state and also look the same. */
        for(i = 0; i < largest_char; ++i) {

            if(is_null(trans_to_states[i])) {
                continue;
            }

            equiv_trans_set = trans_to_states[i];

            if(set_cardinality(equiv_trans_set) < 2) {
                set_empty(equiv_trans_set);
                continue;
            }

            D(
               printf(
                   "\t\t found group of states to investigate on input '%c'. \n",
                   (char) i
               );
            )

            /* we now need to do (c choose 2) comparisons to figure out which
             * have equivalent out-bound transitions. The way we can do these
             * comparisons is by looking at what's on the top of the stack and
             * then comparing it to all states below it. If we find two states
             * with equivalent out-bound transitions then we merge them. We
             * will do the following for each loop of that comparison:
             *
             * There are three special cases to consider when comparing out-
             * bound transitions:
             *
             *    i) self-loops.
             *    ii) one self-loop and one transition to the other state.
             *    ii) one state is accepting and the other is not.
             *
             * We will use a table + three outer loops instead of two nested.
             * The first loop populates the table with trans->state_to =>
             * trans->condition.value. The second outer loop then goes over its
             * transitions and checks its transitions against the table. The
             * final and third outer loop undoes the changes made to the table.
             */

            test_stack.ptr = test_stack.bottom;
            test_stack.top = test_stack.bottom + NFA_MAX_EPSILON_STACK;

            set_map(
                equiv_trans_set,
                (void *) &test_stack,
                (PSetMapFunc *) &NFA_state_stack_push
            );
            set_empty(equiv_trans_set);

            stack_top = test_stack.ptr;
            j = stack_top - test_stack.bottom;

            D( printf("\t\t beginning investigation.. \n"); )

            /* > 0 to ignore what's on the bottom of the stack */
            for(; --j > 0; ) {

                k = *(--test_stack.ptr);

                D( printf("\t\t\t looking for states to merge... \n"); )

                trans = all_transitions + k;
                state_a = trans->from_state;
                trans = all_sources[state_a];

                D( printf("\t\t\t base state is %d \n", state_a); )

                for(; is_not_null(trans); trans = trans->trans_next) {
                    state_trans_values[trans->to_state] = trans->condition.value;
                }

                /* compare the states of the other transitons below the current
                 * transition on the top of the stack. */
                k = test_stack.ptr - test_stack.bottom;
                test_stack_next = test_stack.ptr;

                for(; --k >= 0; ) {
                    --test_stack_next;

                    if(set_has_elm(merged_set, *test_stack_next)) {
                        continue;
                    }

                    trans = all_transitions + *test_stack_next;
                    state_b = trans->from_state;

                    if(state_a == state_b) {
                        continue;
                    }

                    D(
                       printf(
                           "\t\t\t\t seeing if state %d can merge with base "
                           "state %d \n",
                           state_b,
                           state_a
                       );
                    )

                    /* condition iii: one state is accepting and the
                     * other is not. */
                    if(set_has_elm(astates, state_a)
                    != set_has_elm(astates, state_b)) {
                        goto states_distinguishable;
                    }

                    trans = all_sources[state_b];

                    for(; is_not_null(trans); trans = trans->trans_next) {

                        /* easiest condition, we have found two transitions to
                         * the same state on the same value. */
                        if(trans->condition.value
                        == state_trans_values[trans->to_state]) {
                            continue;
                        }

                        /* condition i: self-loops */
                        if(trans->to_state == trans->from_state) {

                            /* does state_a have a self-loop on the same
                             * value? */
                            if(state_trans_values[state_a]
                            == trans->condition.value) {
                                continue;
                            }

                            /* does state_a have a transition to state_b on
                             * the same value? */
                            if(state_trans_values[state_b]
                            == trans->condition.value) {
                                continue;
                            }

                            goto states_distinguishable;
                        }

                        /* condition ii: transition to the other state */
                        if(trans->to_state == state_a) {

                            /* does state_a have a self-loop on the same
                             * value? */
                            if(state_trans_values[state_a]
                            == trans->condition.value) {
                                continue;
                            }

                            goto states_distinguishable;
                        }

                        goto states_distinguishable;
                    }

                    D(
                       printf(
                           "\t\t\t\t states %d and %d can be merged! \n",
                           state_a,
                           state_b
                       );
                    )

                    /* turn any transitions entering state_a into transitions
                     * entering state_b, except for self-transitions on
                     * state_a.
                     *
                     * Note: this will turn transitions from state_b->state_a
                     *       into self-transitions on state_b, as desired. */
                    trans = all_destinations[state_a];
                    for(; is_not_null(trans); trans = trans_next) {

                        trans_next = trans->dest_next;

                        /* ignore self-transition. */
                        if(trans->from_state == state_a) {
                            continue;
                        }

                        /* add in the transitions */
                        trans->to_state = state_b;
                        trans->dest_next = all_destinations[state_b];
                        all_destinations[state_b] = trans;
                    }

                    /* drop all transitions leaving state_a */
                    trans = all_sources[state_a];
                    for(; is_not_null(trans); trans = trans->trans_next) {
                        trans->type = T_UNUSED;

                        /* undo changes while we're at it */
                        state_trans_values[trans->to_state] = -1;
                    }

                    NFA_mark_unused_state(dfa, state_a);
                    set_add_elm(merged_set, state_a);
                    set_remove_elm(astates, state_a);

                    D( printf("\t\t\t\t merged states. \n"); )

                    goto investigate_next_state;

states_distinguishable:
                    continue;
                }

                /* undo the changes made to to the state_trans_values table */
                trans = all_sources[state_a];
                for(; is_not_null(trans); trans = trans->trans_next) {
                    state_trans_values[trans->to_state] = -1;
                }

investigate_next_state:

                D( printf("\t\t\t done. \n"); )

                continue;
            }

            /* go over all of the states and add all of the non-merged states
             * to the stack of states to test later in the run. */
            for(test_stack.ptr = stack_top;
                --test_stack.ptr >= test_stack.bottom;) {

                state_a = (all_transitions + *test_stack.ptr)->from_state;

                if(!set_has_elm(merged_set, state_a)
                && !set_has_elm(seen_set, state_a)) {
                    D( printf("\t\t\t Adding state %d to stack. \n", state_a); )
                    NFA_state_stack_push(&stack, state_a);
                } else {
                    D( printf("\t\t\t Didn't add state %d to stack. \n", state_a); )
                }
            }

            /* we're done with these set, clear them out */
            D( printf("\t\t done testing group. \n"); )
        }

        D( printf("\t done testing groups. \n\n"); )
    }

clean_up:

    for(i = 0; i < largest_char; ++i) {
        if(is_not_null(trans_to_states[i])) {
            set_free(trans_to_states[i]);
        }
    }

    if(malloced_trans_to_states) {
        mem_free(trans_to_states);
    }

    mem_free(state_trans_values);

    set_free(seen_set);
    set_free(merged_set);

    D( printf("done. \n"); )
}

typedef struct DFA_StateTuple {
    unsigned int state_a,
                 state_b;
    int is_marked;
} DFA_StateTuple;

static PNFA *DFA_global_minimize(PNFA *dfa, int largest_char) {

    PNFA *min_dfa = nfa_alloc();

    PSet *astates = dfa->accepting_states;

    char *mark_table,
         *row,
         *cell,
         *cell_p,
         *temp;

    NFA_Transition *trans,
                   **source_transitions,
                   *transitions = (NFA_Transition *) dfa->transitions;

    const int num_states = dfa->num_states,
              num_transitions = dfa->num_transitions;

    int i = dfa->num_transitions,
        j,
        k,
        m,
        *state_trans_values;

    const long int jp = num_states - 1,
                   jjp = num_states * jp;

    const size_t mark_table_size = (sizeof(char) * num_states) * num_states;

    D( printf("configuring global minimize.. \n"); )

    mark_table = mem_alloc(mark_table_size);
    state_trans_values = mem_alloc(sizeof(int) * ++largest_char);

    if(is_null(mark_table) || is_null(state_trans_values)) {
        mem_error("Internal DFA Error: Unable to minimize DFA.");
    }

    source_transitions = (NFA_Transition **) dfa->state_transitions;
    memset(state_trans_values, -1, sizeof(int) * largest_char);

    D( printf("marking initial bad transitions... \n"); )

    /* populate the table and mark all transitions where one of the states is
     * accepting and the other is not. */
    cell = mark_table;
    for(i = num_states; --i >= 0; ) {
        m = set_has_elm(astates, i);
        for(j = num_states; --j >= 0; ++cell) {
            *cell = (m != set_has_elm(astates, j));
        }
    }

    /**************************************************************/
    cell = mark_table;
    for(i = num_states; --i >= 0; ) {
        printf("%2d | ", i);
        for(j = num_states; --j >= 0; ) {

            D( printf("%2d ", *cell); )
            ++cell;
        }

        D( printf("\n"); )
    }
    printf("     ");
    for(i = num_states; --i >= 0; ) {
        printf("---");
    }
    printf("\n     ");
    for(i = num_states; --i >= 0; ) {
        printf("%2d ", i);
    }
    printf("\n");
    /**************************************************************/


    D( printf("running main algorithm... \n"); )

    do {
        D( printf("marking cells... \n"); )

        m = 0;
        row = cell = mark_table;

        for(i = num_states; --i >= 0; row += num_states) {

            /* fill in the values, this will allow us to compare transitions between
             * any two states. */
            trans = source_transitions[i];
            for(; is_not_null(trans); trans = trans->trans_next) {
                state_trans_values[trans->condition.value] = trans->to_state;
            }

            D( printf("Looking at state %d \n", i); )

            trans = transitions;
            for(j = num_transitions; --j >= 0; ++trans) {

                k = state_trans_values[trans->condition.value];

                cell = row + (num_states - trans->from_state - 1);
                cell_p = mark_table + (jjp - (num_states * trans->from_state) + jp - i);

                if(k < 0 || *cell == 1 || *cell_p == 1) {
                    D( printf("\t marking (%d, %d) \n", i, trans->from_state); )
                    *cell = 1;
                    *cell_p = 1;
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

            trans = source_transitions[i];
            for(; is_not_null(trans); trans = trans->trans_next) {
                state_trans_values[trans->condition.value] = -1;
            }



            /*
            for(j = num_states; --j >= 0; ++cell) {

                if(*cell) {
                    continue;
                }

                trans = source_transitions[j];

                for(; is_not_null(trans); trans = trans->trans_next) {
                    k = state_trans_values[trans->condition.value];

                    if(k < 0) {
                        *cell = 1;
                        continue;
                    }

                    temp = mark_table + (jjp - (num_states * k) + jp - trans->to_state);
                    if(*temp) {
                        ++m;
                        *cell = 1;
                        D( printf("marking (%d, %d) \n", i, j); )
                        break;
                    }
                }
            }

            trans = source_transitions[i];
            for(; is_not_null(trans); trans = trans->trans_next) {
                state_trans_values[trans->condition.value] = -1;
            }
            */
        }
    } while(m > 0);

    D( printf("states marked. \n"); )

    /**************************************************************/
    cell = mark_table;
    for(i = num_states; --i >= 0; ) {
        printf("%2d | ", i);
        for(j = num_states; --j >= 0; ) {

            D( printf("%2d ", *cell); )
            ++cell;
        }

        D( printf("\n"); )
    }
    printf("     ");
    for(i = num_states; --i >= 0; ) {
        printf("---");
    }
    printf("\n     ");
    for(i = num_states; --i >= 0; ) {
        printf("%2d ", i);
    }
    printf("\n");
    /**************************************************************/


    D( printf("cleaning up. \n"); )

    mem_free(state_trans_values);
    mem_free(mark_table);

    return min_dfa;
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

PNFA *nfa_to_dfa(PNFA *nfa) {
    int largest_char;
    PNFA *dfa = NULL;
    assert_not_null(nfa);

    largest_char = NFA_max_alphabet_char(nfa);

    nfa = NFA_subset_construction(nfa, largest_char);

    printf("\n\n");

    nfa_print_dot(nfa);
    printf("\n\n");

    DFA_global_minimize(nfa, largest_char);

    printf("\n\n");

    DFA_local_minimize(nfa, largest_char);
    printf("\n\n");

    /*nfa_free(nfa);*/

    return nfa;
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
                        "Ox%d -> Ox%d [label=< '%c'<FONT POINT-SIZE=\"8\"> %d</FONT> >] \n",
                        state,
                        transition->to_state,
                        (unsigned char) transition->condition.value,
                        transition->id
                    );
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
