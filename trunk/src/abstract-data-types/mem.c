/*
 * mem.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <stdio.h>

inline void *mem_alloc(const int s) {
    return malloc(s);
}

inline void mem_error(const char * const s) {
    printf(s);
    exit(1);
}
