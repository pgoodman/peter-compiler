/*
 * std-string.h
 *
 *  Created on: May 9, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef STDSTRING_H_
#define STDSTRING_H_

#include <math.h>
#include <string.h>

#include "std-include.h"

#define P_STRING_HEAP_START_SIZE 100

typedef char PChar;

typedef struct PString {
    uint32_t len;
    PChar *str;
} PString;

PString *string_alloc_char(const char * const str, const uint32_t len );

void string_free(PString * );

PString *string_copy(PString *str);

int string_equal(const PString * const, const PString * const );

uint32_t string_length(const PString * const );

void string_convert_to_ascii(const PString * const, char * );

void delegate_string_free(void *);

unsigned long int string_num_allocated_pointers(void);

uint32_t string_hash_fnc(PString *str);

int string_collision_fnc(PString *a, PString *b);

#endif /* STDSTRING_H_ */
