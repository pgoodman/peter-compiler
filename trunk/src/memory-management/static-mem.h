/*
 * memory.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef MEMORY_H_
#define MEMORY_H_
#define MEM_DEBUG

#include <stdlib.h>
#include <stdio.h>

/**
 * Deal with debugging heap allocation and freeing.
 */

#ifdef MEM_DEBUG

void *_mem_alloc(size_t, unsigned int, char *);
void *_mem_calloc(size_t, size_t, unsigned int, char *);
void _mem_free(void *, unsigned int, char *);
void _D1_mem_free(void *);

#define mem_calloc(a, b) _mem_calloc(a, b, __LINE__, __FILE__)
#define mem_alloc(size) _mem_alloc(size, __LINE__, __FILE__)
#define mem_free(size) _mem_free(size, __LINE__, __FILE__)
#define D1_mem_free _D1_mem_free

#else

#define mem_calloc calloc
#define mem_alloc malloc
#define mem_free free
#define D1_mem_free free

#endif

#define mem_error(e) { \
    printf(e " in %s on line %d.", __FILE__, (unsigned int)__LINE__); \
    fflush(stdout); \
    exit(1);}

#endif /* MEMORY_H_ */
