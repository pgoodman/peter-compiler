/*
 * scanner.c
 *
 *  Created on: May 13, 2009
 *      Author: petergoodman
 *     Version: $Id$
 *
 * This scanner is heavily influenced by the code in "Compiler Design in C"
 * by Allen I. Holub, edited by Brian W. Kernighan, and published by Prentice-
 * Hall.
 */

#include <p-scanner.h>

#ifdef MSDOS
#   define COPY(d,s,a) memmove(d,s,a)
#else
#   define COPY(d,s,a) memcpy(d,s,a)
#endif

#ifndef O_BINARY
#   define O_BINARY 0
#endif

/* -------------------------------------------------------------------------- */

/**
 * Open a new file for the scanner to use. If the file cannot be opened then
 * 0 is returned, else 1.
 */
static int I_file_open(PScanner *scanner, const char *file_name) {

    int file_descriptor;
    char *end_of_buffer;

    assert_not_null(scanner);
    assert_not_null(file_name);

    /* close any file that was previously open. */
    I_file_close(scanner);

    /* prepare the scanner's buffer for this file. this means putting the end of
     * the buffer just *after* the actual end of the buffer, setting the
     * starting flush point, and making sure any lexeme information that exists
     * in the scanner already has been cleared out. */
    end_of_buffer = (scanner->buffer.start) + S_INPUT_BUFFER_SIZE;

    scanner->buffer.end = end_of_buffer;
    scanner->buffer.flush_point = end_of_buffer - S_MAX_LOOKAHEAD;
    scanner->buffer.next_char = end_of_buffer;

    scanner->lexeme.curr_end = end_of_buffer;
    scanner->lexeme.curr_start = end_of_buffer;
    scanner->lexeme.prev_start = NULL;
    scanner->lexeme.prev_length = 0;
    scanner->lexeme.prev_line_num = 0;
    scanner->lexeme.term_char = 0;

    scanner->input.column = 1;
    scanner->input.line = 1;
    scanner->input.eof_read = 0;

    /* open the file, if the fail opens then */
    MSFT( if(0 == strcmp(file_name, "/dev/tty")) )
    MSFT(     file_name = "CON"; )

    file_descriptor = open(file_name, O_RDONLY | O_BINARY);

    if(-1 == file_descriptor) {
        return 0;
    }

    scanner->input.file_descriptor = file_descriptor;

    return 1;
}

static void I_file_close(PScanner *scanner) {
    assert_not_null(scanner);

    if(-1 != scanner->input.file_descriptor) {
        close(scanner->input.file_descriptor);
        scanner->input.file_descriptor = -1;
    }
}

static void I_file_read(PScanner *scanner) {
    assert_not_null(scanner);
}

/* -------------------------------------------------------------------------- */

static void I_buffer_fill() {

}

/**
 * Shift the contents of the contents of the buffer out until the previous
 * lexeme is at the head of the buffer.
 */
static void I_buffer_flush() {

}

/* -------------------------------------------------------------------------- */

PScanner *scanner_alloc(void) {

}

void scanner_free(PScanner *scanner) {

}
