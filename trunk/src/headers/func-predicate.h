/*
 * func-predicate.h
 *
 *  Created on: Jun 2, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef FUNCPREDICATE_H_
#define FUNCPREDICATE_H_

typedef int (PPredicate)(void *);

int predicate_true(void *X);
int predicate_false(void *X);

#endif /* FUNCPREDICATE_H_ */
