/*
 * std-string.h
 *
 *  Created on: May 9, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef STDSTRING_H_
#define STDSTRING_H_

#include "std-include.h"

#define P_STRING_HEAP_START_SIZE 100

typedef char PChar;

typedef struct PString {
    uint32_t len;
    PChar *str;
} PString;

PString *string_alloc_char(const char * const str, const uint32_t len );
void string_free(PString * );
char string_equal(const PString * const, const PString * const );
uint32_t string_length(const PString * const );
void string_convert_to_ascii(const PString * const, char * );
void delegate_string_free(void *);

#if 0
typedef struct PString {
    PInternalString* string;
    char ref_count;
} PString;

typedef struct PStringHeap {

    /* size for our string and tomb stone heaps.
     */
    int num_internal_strings,
        num_external_strings,

        num_used_external_strings,
        num_used_internal_strings;

    /* various places where different things are stored. */
    PString *external_string_area;
    PInternalString *internal_string_area;

} PStringHeap;
#endif

#endif /* STDSTRING_H_ */
