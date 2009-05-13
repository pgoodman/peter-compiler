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

typedef void *(*PFunction)(void * );
typedef void *(*PFunction2)(void *, void * );

void *function_identity(void * );

#endif /* FUNCTION_H_ */
