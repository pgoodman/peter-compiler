/*
 * mem.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <std-memory.h>


#if defined(P_DEBUG_MEM) && P_DEBUG_MEM == 1

#include <adt-dict.h>
#include <func-delegate.h>

unsigned int num_allocated_pointers = 0;

void *_mem_calloc(size_t s, size_t e, unsigned int line, char *file ) { $H
    void *x = calloc(s, e);

    ++num_allocated_pointers;
    printf("Allocated memory address 0x%X, %d:%s, loose pointers remaining: %d.\n", (int)x, line, file, num_allocated_pointers);
    fflush(stdout);

    return_with x;
}

void *_mem_alloc(size_t s, unsigned int line, char *file ) { $H
    void *x = malloc(s);

    ++num_allocated_pointers;
    printf("Allocated memory address 0x%X, %d:%s, loose pointers remaining: %d.\n", (int)x, line, file, num_allocated_pointers);
    fflush(stdout);

    return_with x;
}

void _mem_free(void *x, unsigned int line, char *file ) { $H
    --num_allocated_pointers;
    printf("Freeing memory address 0x%X, %d:%s, loose pointers remaining: %d.\n", (int)x, line, file, num_allocated_pointers);
    fflush(stdout);

    free(x);
    return_with;
}

void _D1_mem_free(void *x ) { $H

    --num_allocated_pointers;
    printf("Freeing memory address 0x%X, loose pointers remaining: %d.\n", (unsigned int)x, num_allocated_pointers);
    fflush(stdout);

    free(x);
    return_with;
}

#else

void _D1_mem_free(void *x ) { $H
    free(x);
    return_with;
}

#endif

