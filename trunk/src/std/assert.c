/*
 * assert.c
 *
 *  Created on: May 9, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <std-assert.h>

/**
 * Print a backtrace for assertions.
 */
void assert_print_stack_trace(StackTrace *T, unsigned int line, char *file) {

    printf("Assertion failed on line %d in %s:\n", line, file);
    while(NULL != T) {
        printf("\t%s one line %d\n", T->file, T->line);
        T = T->next;
    }

    exit(1);
}
