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

typedef void (*PDelegate)(void * );
typedef void (*PDelegate2)(void *, void * );

void delegate_do_nothing(void * const);

#endif /* DELEGATE_H_ */
