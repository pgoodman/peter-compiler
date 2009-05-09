/*
 * delegate.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef DELEGATE_H_
#define DELEGATE_H_

#include "std-include.h"

typedef void (*D1_t)(void * $$);
typedef void (*D2_t)(void *, void * $$);

void D1_ignore(void * const $$);

#endif /* DELEGATE_H_ */
