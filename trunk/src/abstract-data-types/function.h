/*
 * function.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef FUNCTION_H_
#define FUNCTION_H_

typedef void *(*F1)(void *);
typedef void *(*F2)(void *, void *);

void *F1_identity(void *);

#endif /* FUNCTION_H_ */
