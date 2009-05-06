/*
 * mem.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <stdio.h>
#include "mem.h"

#ifdef MEM_DEBUG
unsigned int num_loose_pointers = 0;
#endif

void *mem_alloc(const int s MEM_DEBUG_PARAMS) {
#ifdef MEM_DEBUG
    ++num_loose_pointers;
    void *x = malloc(s);
    printf("Allocated memory address 0x%X, loose pointers remaining: %d.\n", (int)x, num_loose_pointers);
    fflush(stdout);
    return x;
#else
    return malloc(s);
#endif

}

void mem_free(void *x MEM_DEBUG_PARAMS) {
#ifdef MEM_DEBUG
    --num_loose_pointers;
    printf("Freeing memory address 0x%X, line %d, file %s, loose pointers remaining: %d.\n", (unsigned int)x, line, file, num_loose_pointers);
    fflush(stdout);
#endif

    free(x);
}

inline void mem_error(const char * const s) {
    printf(s);
    exit(1);
}
