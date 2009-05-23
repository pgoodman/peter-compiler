/*
 * debug.h
 *
 *  Created on: May 9, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include "std-include.h"

typedef struct PStackTrace {
    struct PStackTrace *next;
    char *file;
    unsigned int line;
} PStackTrace;

#if defined(P_DEBUG) && P_DEBUG == 1

#define std_error(e) { \
    fprintf(stderr, e " in %s on line %d.\n", __FILE__, (unsigned int)__LINE__); \
    fflush(stdout); \
    exit(1);}

#else
#define std_error(e) exit(1);

#endif /* DEBUG */

#endif /* DEBUG_H_ */
