/*
 * assert.h
 *
 *  Created on: May 9, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef ASSERT_H_
#define ASSERT_H_

#include <stdlib.h>
#include <stdio.h>
#include "std-debug.h"

#if defined(P_DEBUG) && P_DEBUG == 1

void assert_print_stack_trace(StackTrace *, unsigned int, char *);

#define assert(e) {if(!(e)){\
    (void)assert_print_stack_trace(_ST,__LINE__,__FILE__);}}

#else

#define assert(e)

#endif /* debug */

#endif /* ASSERT_H_ */
