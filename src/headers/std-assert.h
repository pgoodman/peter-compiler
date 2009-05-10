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

typedef enum {
    P_ASSERT_NORMAL,
    P_ASSERT_NOT_NULL
} PAssertionType;

#if defined(P_DEBUG) && P_DEBUG == 1

void assert_print_stack_trace(PAssertionType, PStackTrace *, unsigned int, char *);

#define _assert(e,type) {if(!(e)){\
    (void)assert_print_stack_trace(type,_ST,__LINE__,__FILE__);}}

#define assert(e) _assert((e),P_ASSERT_NORMAL)
#define assert_not_null(e) _assert((NULL!=(e)),P_ASSERT_NOT_NULL)

#else

#define assert(e)

#endif /* debug */

#endif /* ASSERT_H_ */
