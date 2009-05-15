/*
 * string.c
 *
 *  Created on: May 10, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <std-string.h>

/**
 * Allocate space a new string on the heap.
 */
static PString *string_alloc(uint32_t len ) {
    assert(len > 0);

    PString *S = mem_alloc(sizeof(PString)+(sizeof(PChar) * len));
    if(NULL == S) {
        mem_error("Unable to allocate string on heap.");
    }

    S->len = len;
    S->ref_count = 1;
    S->str = (PChar *) (((char *) S) + (sizeof(PString) / sizeof(char)));

    return S;
}

static PChar S_char_to_pchar(char c ) {
    return (PChar) c;
}

/**
 * Allocate a new string on the heap by using a stack-allocated array of char.
 */
PString *string_alloc_char(const char * const str, const uint32_t len ) {
    PString *S;

    S = string_alloc(len );
    int i;

    /* copy the old characters, including null character, into the
     * heap-allocated chars */
    for(i = 0; i < len+1; ++i)
        S->str[i] = S_char_to_pchar(str[i] );

    return S;
}

/**
 * Return the string length.
 */
uint32_t string_length(const PString * const S ) {
    assert_not_null(S);
    return S->len;
}

/**
 * This will convert the PString to ascii. It assumes a stack-allocated character
 * array to put the ascii characters into that is of proper + 1 for terminating
 * character.
 */
void string_convert_to_ascii(const PString * const S, char *ascii_version ) {
    assert_not_null(S);
    assert_not_null(ascii_version);

    uint32_t i;

    for(i = 0; i < S->len; ++i)
        ascii_version[i] = S->str[i];

    ascii_version[S->len] = 0;
    return;
}

/**
 * Increase the refcount on a string. TODO: overflow check?
 */
void string_use(PString *S ) {
    assert_not_null(S);
    ++(S->ref_count);
    return;
}

/**
 * Either free a string or decrease its reference counter.
 */
void string_free(PString *S ) {
    assert_not_null(S);

    if(S->ref_count <= 1) {
        mem_free(S);
    } else {
        --(S->ref_count);
    }
    return;
}

void delegate_string_free(void *str ) {
    assert_not_null(str);
    string_free(((PString *) str) );
    return;
}

/**
 * Check if two strings contain the same characters.
 */
char string_equal(const PString * const A, const PString * const B ) {
    assert_not_null(A);
    assert_not_null(B);

    char eql = 0;
    int i,
        len = A->len;

    if(len == B->len) {

        PChar *a = A->str,
              *b = B->str;

        for(i = 0; i < len && a[i] == b[i]; ++i)
            ;

        eql = (i == len);
    }

    return eql;
}

#if 0
PStringHeap *string_heap_alloc(void) {

    /* try to allocate the object with which we will use to store different sets
     * of strings. */
    PStringHeap *heap = mem_alloc(sizeof(PStringHeap));
    if(NULL == heap) {
        mem_error("Unable to allocate new string heap.");
    }

    /* try to allocate the starting heap space for the strings */
    PInternalString *pointer_heap = mem_calloc( \
        sizeof(PInternalString), \
        P_STRING_HEAP_START_SIZE \
    );

    if(NULL == pointer_heap) {
        mem_error("Unable to allocate heap space for a new string heap.")
    }

    /* try to allocate the starting heap space for the tomb stones. */
    PString *tombstone_heap = mem_calloc( \
        sizeof(PString), \
        P_STRING_HEAP_START_SIZE \
    );

    heap->internal_string_area = pointer_heap;
    heap->external_string_area = tombstone_heap;

    heap->num_internal_strings = P_STRING_HEAP_START_SIZE;
    heap->num_external_strings = P_STRING_HEAP_START_SIZE;

    heap->num_used_internal_strings = 0;
    heap->num_used_external_strings = 0;

    return heap;
}

static PInternalString *string_internal_alloc(PStringHeap *H ) {
    PInternalString *IS;

    return IS;
}

static PString *string_external_alloc(PStringHeap *H ) {
    PString *S;

    // try to either re-allocate space.
    if(H->num_used_external_strings >= H->num_external_strings) {
        H->external_string_area = mem_realloc( \
            H->external_string_area, \
            (H->num_external_strings * 2) \
        );

        if(NULL == H->external_string_area) {
            mem_error("Unable to expand external string heap.");
        }
    }

    ++(H->num_used_external_strings);

    return S;
}

static PInternalChar *string_char_alloc(uint32_t len ) {
    PInternalChar C = mem_alloc(sizeof(PInternalChar) * len);

    if(NULL == C) {
        mem_error("Unable to allocate string on heap.");
    }

    return C;
}

PString *string_alloc(PStringHeap *H, int str_len ) {
    PString *S;
    PInternalString *IS;

    assert_not_null(H);

    return S;
}

#endif
