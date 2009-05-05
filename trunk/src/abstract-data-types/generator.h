/*
 * generator.h
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef GENERATOR_H_
#define GENERATOR_H_

typedef struct Generator {
    F1 generate;
    D1 free;
} Generator;

void *generator_alloc(int);

#endif /* GENERATOR_H_ */
