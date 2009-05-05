/*
 * memory.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef MEMORY_H_
#define MEMORY_H_

// function type to report a heap allocation error
typedef int (*mem_error_fnc)(const char * const);

// allocate memory on the heap
void mem_error(const char * const);
void *mem_alloc(const int);

#endif /* MEMORY_H_ */
