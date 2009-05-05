/*
 * mem.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <stdlib.h>
#include <stdio.h>

void *mem_alloc(const int s) {
    return malloc(s);
}

void mem_free(void *x) {
    free(x);
}

inline void mem_error(const char * const s) {
    printf(s);
    exit(1);
}
