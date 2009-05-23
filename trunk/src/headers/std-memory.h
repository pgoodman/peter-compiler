/*
 * memory.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "std-include.h"

/**
 * Deal with debugging heap allocation and freeing.
 */

#if defined(P_DEBUG_MEM) && P_DEBUG_MEM == 1

void *_mem_alloc(size_t, const unsigned int, const char * );
void *_mem_calloc(size_t, size_t, const unsigned int, const char * );
void _mem_free(void *, const unsigned int, const char * );
void _D1_mem_free(void * );

#define mem_calloc(a, b) _mem_calloc((a), (b), __LINE__, __FILE__)
#define mem_alloc(size) _mem_alloc((size), __LINE__, __FILE__)
#define mem_free(size) _mem_free((size), __LINE__, __FILE__)
#define delegate_mem_free _D1_mem_free


#else

#define mem_calloc calloc
#define mem_alloc malloc
#define mem_free free

#if defined(P_DEBUG) && P_DEBUG == 1
void _D1_mem_free(void * );
#define delegate_mem_free _D1_mem_free
#else
#define delegate_mem_free free
#endif

#endif /* debug memory defined */

#define mem_realloc realloc

#define mem_error(e) { \
    fprintf(stderr, e " in %s on line %d.\n", __FILE__, (unsigned int)__LINE__); \
    exit(1);}

#endif /* MEMORY_H_ */
