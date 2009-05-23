/*
 * input.c
 *
 *  Created on: May 10, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#include <std-input.h>

/**
 * Allocate and return pointer to a file.
 */
PFileInputStream *file_read(const PString * const path) {
    char *path_char = alloca(string_length(path)+1);
    string_convert_to_ascii(path, path_char);
    return file_read_char(path_char);
}
PFileInputStream *file_read_char(const char * const path) {
    FILE *file = NULL;
    PFileInputStream *stream = NULL;

    assert_not_null(path);

    stream = mem_alloc(sizeof(PFileInputStream));
    if(is_null(stream)) {
        mem_error("Unable to allocate a new file input stream on the heap.");
    }

    file = fopen(path, "r");
    if(is_null(file)) {
        std_error("Unable to open/read file.");
    }

    stream->ptr = file;

    setvbuf(file, stream->buff, _IOFBF, 4096);

    return stream;
}

/**
 * Free a file pointer.
 */
void file_free(PFileInputStream *stream) {
    assert_not_null(stream);

    fclose(stream->ptr);
    stream->ptr = NULL;
    mem_free(stream);
    return;
}

/**
 * Read a character from a file.
 */
unsigned char file_get_char(PFileInputStream *stream) {
    assert_not_null(stream);
    assert_not_null(stream->ptr);

    return fgetc(stream->ptr);
}
