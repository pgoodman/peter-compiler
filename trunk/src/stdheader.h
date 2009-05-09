/*
 * stdheader.h
 *
 *  Created on: May 8, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef STDHEADER_H_
#define STDHEADER_H_

#include <stdlib.h>
#include <assert.h>
#include "vendor/pstdint.h"
#include "memory-management/static-mem.h"

#define std_error(e) { \
    printf(e " in %s on line %d.", __FILE__, (unsigned int)__LINE__); \
    fflush(stdout); \
    exit(1);}

#endif /* STDHEADER_H_ */
