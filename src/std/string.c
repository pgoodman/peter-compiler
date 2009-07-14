/*
 * string.c
 *
 *  Created on: May 10, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <std-string.h>

static unsigned long int num_allocations = 0;

#define string_mem_alloc(x) mem_alloc(x); ++num_allocations
#define string_mem_calloc(x,y) mem_calloc(x,y); ++num_allocations
#define string_mem_free(x) mem_free(x); --num_allocations
#define string_mem_error(x) mem_error(x)

unsigned long int string_num_allocated_pointers(void) {
    return num_allocations;
}

/**
 * Allocate space a new string on the heap.
 */
static PString *string_alloc(uint32_t len) {
    PString *S;

    S= string_mem_alloc(sizeof(PString)+(sizeof(PChar) * (len+1)));
    if(NULL == S) {
        mem_error("Unable to allocate string on heap.");
    }

    S->len = len;
    S->str = (PChar *) (((char *) S) + (sizeof(PString) / sizeof(char)));
    S->str[len] = 0;

    return S;
}

/**
 * Copy an existing string.
 */
PString *string_copy(PString *str) {
    PString *copy;

    assert_not_null(str);

    copy = string_alloc(str->len);
    copy->str = memcpy(copy->str, str->str, (str->len + 1) * sizeof(PChar));

    return copy;
}

static PChar S_char_to_pchar(char c ) {
    return ((PChar) c);
}

/**
 * Allocate a new string on the heap by using a stack-allocated array of char.
 */
PString *string_alloc_char(const char *str, const uint32_t len ) {
    PString *S;
    uint32_t i;

    S = string_alloc(len);

    /* copy the old characters, including null character, into the
     * heap-allocated chars */
    S->str = memcpy(S->str, str, (len + 1) * sizeof(PChar));
    S->str[len] = 0;

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
    uint32_t i;

    assert_not_null(S);
    assert_not_null(ascii_version);

    for(i = 0; i < S->len; ++i) {
        ascii_version[i] = S->str[i];
    }

    ascii_version[S->len] = 0;
    return;
}

/**
 * Either free a string or decrease its reference counter.
 */
void string_free(PString *S ) {
    assert_not_null(S);
    string_mem_free(S);
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
int string_equal(const PString * const A, const PString * const B ) {
    uint32_t i = 0,
             len;
    PChar *a = NULL,
          *b = NULL;

    assert_not_null(A);
    assert_not_null(B);

    len = A->len;

    if(len == B->len) {
        for(a = A->str, b = B->str, i = 0; i < len && a[i] == b[i]; ++i)
            ;
    }

    return (i == len);
}

/**
 * Hash a string into an int.
 */
uint32_t string_hash_fnc(PString *str) {
   int n;
   uint32_t hashval;
   unsigned int i, ival;
   char *p;
   p = (char *) &ival;
   hashval = ival = 0;
   n = (((log10((double)(UINT_MAX)) / log10(2.0))) / CHAR_BIT) + 0.5;
   for(i = 0; i < str->len; i += n) {
      strncpy(p, str->str + i, n);
      hashval += ival;
   }

   return hashval;
}

/**
 * Check If two strings are different.
 */
int string_collision_fnc(PString *a, PString *b) {
    return !string_equal(a, b);
}
