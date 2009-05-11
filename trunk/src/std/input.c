/*
 * input.c
 *
 *  Created on: May 10, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <std-input.h>
#include <std-string.h>

/**
 * Allocate and return pointer to a file.
 */
PFile *file_alloc(const char * const path, const char * const how_to_open $$) { $H
    assert_not_null(path);
    assert_not_null(how_to_open);

    FILE *F = fopen(path, how_to_open);

    if(NULL == F) {
        mem_error("Unable to allocate new file pointer.");
    }

    return_with (PFile *) F;
}

/**
 * Free a file pointer.
 */
void file_free(PFile *F $$) { $H
    fclose((FILE *) F);
    return_with;
}
