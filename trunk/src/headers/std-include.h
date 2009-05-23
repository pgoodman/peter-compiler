/*
 * stdheader.h
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#define P_DEBUG 1
#define P_DEBUG_MEM 1
#define P_DEBUG_PRINT_TRACE 0

#ifndef STDHEADER_H_
#define STDHEADER_H_

#define is_null(x) (NULL == (x))
#define is_not_null(x) (NULL != (x))

#include <stdlib.h>
#include <stdio.h>
#include "vendor-pstdint.h"
#include "std-debug.h"
#include "std-assert.h"
#include "std-memory.h"
#include "std-string.h"

#endif /* STDHEADER_H_ */
