/*
 * assert.c
 *
 *  Created on: May 9, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <std-include.h>
#include <std-assert.h>

#if defined(P_DEBUG) && P_DEBUG == 1

/**
 * Print a backtrace for assertions.
 */
void assert_print_stack_trace(PAssertionType type,
                              unsigned int line,
                              const char *file) {
    const char *error;

    if(P_ASSERT_NORMAL == type) {
        error = "Assertion failed";
    } else if(P_ASSERT_NOT_NULL == type) {
        error = "Null pointer error";
    } else if(P_ASSERT_NULL == type) {
        error = "Non-null error";
    } else {
        error = "";
    }

    fprintf(stderr,"%s on line %d in %s:\n", error, line, file);
    exit(1);
}

#endif
