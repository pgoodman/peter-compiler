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
                              PStackTrace *T,
                              unsigned int line,
                              char *file) {
    char *error = "";

    switch(type) {
        case P_ASSERT_NORMAL:
            error = "Assertion failed";
            break;
        case P_ASSERT_NOT_NULL:
            error = "Null pointer error";
            break;
    }

    printf("%s on line %d in %s:\n", error, line, file);
    while(NULL != T) {
        printf("\t%s one line %d\n", T->file, T->line);
        T = T->next;
    }

    exit(1);
}

#endif
