/*
 * delegate.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef DELEGATE_H_
#define DELEGATE_H_

typedef void (*D1)(void *);
typedef void (*D2)(void *, void *);

void D1_ignore(void * const);

#endif /* DELEGATE_H_ */
