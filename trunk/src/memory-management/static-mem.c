/*
 * mem.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include "static-mem.h"

#ifdef MEM_DEBUG
#include <stdio.h>
unsigned int num_allocated_pointers = 0;

void *_mem_calloc(size_t s, size_t e, unsigned int line, char *file) {
    ++num_allocated_pointers;
    void *x = calloc(s, e);
    printf("Allocated memory address 0x%X, line %d, file %s, loose pointers remaining: %d.\n", (int)x, line, file, num_allocated_pointers);
    fflush(stdout);
    return x;
}

void *_mem_alloc(size_t s, unsigned int line, char *file) {
    ++num_allocated_pointers;
    void *x = malloc(s);
    printf("Allocated memory address 0x%X, line %d, file %s, loose pointers remaining: %d.\n", (int)x, line, file, num_allocated_pointers);
    fflush(stdout);
    return x;
}

void _mem_free(void *x, unsigned int line, char *file) {
    --num_allocated_pointers;
    printf("Freeing memory address 0x%X, line %d, file %s, loose pointers remaining: %d.\n", (int)x, line, file, num_allocated_pointers);
    fflush(stdout);
    free(x);
}

void _D1_mem_free(void *x) {
    --num_allocated_pointers;
    printf("Freeing memory address 0x%X, loose pointers remaining: %d.\n", (unsigned int)x, num_allocated_pointers);
    fflush(stdout);
    free(x);
}

#endif

