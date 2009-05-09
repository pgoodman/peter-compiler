/*
 * function.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "std-include.h"

typedef void *(*F1_t)(void * $$);
typedef void *(*F2_t)(void *, void * $$);

void *F1_identity(void * $$);

#endif /* FUNCTION_H_ */
