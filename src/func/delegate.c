/*
 * delegate.c
 *
 *  Created on: May 5, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <func-delegate.h>

/**
 * An empty delegate on one variable.
 */
void delegate_do_nothing(void *E) {

}

/* Free a pointer. */
void delegate_free_pointer(void *pointer) {
    mem_free(pointer);
}
