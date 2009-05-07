/*
 * memory.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef MEMORY_H_
#define MEMORY_H_

//#  define MEM_DEBUG

/**
 * Deal with debugging heap allocation and freeing.
 */
#ifdef MEM_DEBUG
#define MEM_DEBUG_INFO , __LINE__, __FILE__
#define MEM_DEBUG_PARAMS , int line, char* file
#else
#define MEM_DEBUG_INFO
#define MEM_DEBUG_PARAMS
#endif

#include <stdlib.h>
#include <stdio.h>

// function type to report a heap allocation error
typedef int (*mem_error_fnc)(const char * const);

// allocate memory on the heap
void mem_error(const char * const);
void *mem_alloc(const int MEM_DEBUG_PARAMS);
void mem_free(void * MEM_DEBUG_PARAMS);
void mem_free_no_debug(void *);

#endif /* MEMORY_H_ */
