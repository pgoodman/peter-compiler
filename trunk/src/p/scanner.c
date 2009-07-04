/*
 * scanner.c
 *
 *  Created on: May 13, 2009
 *      Author: petergoodman
 *     Version: $Id$
 *
 * Parts of this scanner are either heavily influenced by or adapted from the
 * code in "Compiler Design in C" by Allen I. Holub, edited by Brian W.
 * Kernighan, and published by Prentice-Hall.
 *
 * This scanner is strictly forward-looking. Once a request is made on the
 * canner to
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

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define NO_MORE_CHARS(s) \
    ((s)->input.eof_read && (s)->buffer.next_char > (s)->buffer.end)

#define PAST_FLUSH_POINT(s) \
    ((s)->buffer.next_char >= ((s)->buffer.end - S_MAX_LOOKAHEAD))

/* -------------------------------------------------------------------------- */

/**
 * Close any open file descriptors.
 */
static void I_close(PScanner *scanner) {
    if(-1 != scanner->input.file_descriptor) {
        close(scanner->input.file_descriptor);
        scanner->input.file_descriptor = -1;
    }
}

/**
 * Read a chunk of data from the file into the scanner's buffer.
 */
static int I_read(PScanner *scanner, unsigned char *start, int how_much) {
    return read(scanner->input.file_descriptor, start, how_much);
}


/* -------------------------------------------------------------------------- */

/**
 * Fill up the buffer, starting from 'starting_from' until the end of the buffer.
 * If this is the first fill of the buffer then it starts at the second
 * character in the buffer and makes the first character a newline.
 *
 * Buffers are read in units of S_MAX_LEXEME_LENGTH. If that many characters
 * cannot be read (returns 0).
 */
static int B_fill(PScanner *scanner, unsigned char *starting_from) {

    int need,
        got,
        total;

    unsigned char *end = scanner->buffer.end;

    /*
    if(!scanner->input.line) {
        *starting_from = '\n';
        ++starting_from;
    }*/

    if(end < starting_from) {
        std_error("Internal Scanner Error: trying to fill buffer beyond buffer.");
    }

    need = ((end - starting_from) / S_MAX_LEXEME_LENGTH) * S_MAX_LEXEME_LENGTH;

    if(0 == need) {
        return 0;
    }

    got = I_read(scanner, starting_from, need);

    if(-1 == got) {
        std_error("Unable to read from input file or read interrupted.");
    }

    total = got;

    /* this may indicate that we've reached the end of the file OR that read
     * read some data but was then interrupted. We'll try to get the rest and
     * if we can't then we'll assume that we've reached the end of the file. */
    if(got < need) {
        starting_from += got;
        need -= got;

        got = I_read(scanner, starting_from, need);
        if(0 >= got) {
            scanner->input.eof_read = 1;
        } else {
            total += got;
        }
    }

    scanner->buffer.end = starting_from + got;

    return total;
}

/**
 * Shift the contents of the contents of the buffer out until the previous
 * lexeme is at the head of the buffer. If the current character isn't past the
 * flush point then no flush will occur unless it is forced.
 *
 * Returns
 *     -1 - The buffer is too full to be flushed.
 *      0 - We're at the end of the file.
 *      1 - Everything is okay. A flush may or may not have been executed.
 */
int scanner_flush(PScanner *scanner, int force_flush) {

    unsigned int copy_amount = 0,
                 shift_amount = 0;

    unsigned char *left_edge,
                  *buffer_start = scanner->buffer.start,
                  *buffer_end = scanner->buffer.end;

    if(NO_MORE_CHARS(scanner)) {
        return 0;
    }

    if(scanner->input.eof_read) {
        return 1;
    }

    if(PAST_FLUSH_POINT(scanner) || force_flush) {

        left_edge = scanner->lexeme.start;
        shift_amount = left_edge - buffer_start;

        /* we're not adding enough room to the buffer in order to accommodate a
         * lexeme of unusual length. */
        if(shift_amount < S_MAX_LEXEME_LENGTH) {

            if(!force_flush) {
                return -1;
            }

            /* this effectively destroys the current lexeme and annihilates the
             * previous one, setting the parser into a similar state to what
             * it would be in if it had just opened a file. */
            left_edge = scanner_mark_lexeme_start(scanner);
            shift_amount = left_edge - buffer_start;
        }

        /* shift the buffer */
        copy_amount = buffer_end - left_edge;
        COPY(buffer_start, left_edge, copy_amount);

        /* fill in the now free area of the buffer */
        if(!B_fill(scanner, buffer_start + copy_amount)) {
            std_error("Internal Error: Cannot fill buffer, buffer is full.");
        }

        /* this works even if we forced the buffer full because we used
         * L_current_mark_start to move the current lexeme's pointer information
         * to the next char in the buffer as that becomes the shift boundary. */
        scanner->lexeme.start -= shift_amount;
        scanner->lexeme.end -= shift_amount;
        scanner->buffer.next_char -= shift_amount;
    }

    return 1;
}

/* -------------------------------------------------------------------------- */

/**
 * Allocate a new scanner on the heap and return it.
 */
PScanner *scanner_alloc(void) {
    PScanner *scanner = mem_alloc(sizeof(PScanner));
    if(is_null(scanner)) {
        mem_error("Unable to heap-allocate a new scanner.");
    }
    return scanner;
}

/**
 * Free a scanner.
 */
void scanner_free(PScanner *scanner) {
    assert_not_null(scanner);
    I_close(scanner);
    mem_free(scanner);
}

/**
 * Open a new file for the scanner to use. If the file cannot be opened then
 * 0 is returned, else 1.
 */
int scanner_open(PScanner *scanner, const char *file_name) {

    int file_descriptor;
    unsigned char *end_of_buffer;

    assert_not_null(scanner);
    assert_not_null(file_name);

    /* close any file that was previously open. */
    I_close(scanner);

    /* prepare the scanner's buffer for this file. this means putting the end of
     * the buffer just *after* the actual end of the buffer, setting the
     * starting flush point, and making sure any lexeme information that exists
     * in the scanner already has been cleared out. */
    end_of_buffer = scanner->buffer.start + S_INPUT_BUFFER_SIZE;

    scanner->buffer.end = end_of_buffer;
    scanner->buffer.flush_point = (end_of_buffer - S_MAX_LOOKAHEAD);
    scanner->buffer.next_char = end_of_buffer;

    scanner->lexeme.end = end_of_buffer;
    scanner->lexeme.start = end_of_buffer;
    scanner->lexeme.as_string = NULL;

    scanner->input.column = 1;
    scanner->input.line = 0;
    scanner->input.eof_read = 0;

    /* open the file, if the fail opens then */
    file_descriptor = open(file_name, O_RDONLY | O_BINARY);
    if(-1 == file_descriptor) {
        return 0;
    }

    scanner->input.file_descriptor = file_descriptor;

    return 1;
}

/**
 * Return the next input character in the buffer and then advance the buffer
 * past it. Returns 0 if end-of-file is reached. Returns -1 if the buffer is
 * too full to be flushed.
 */
char scanner_advance(PScanner *scanner) {
    char next;

    if(NO_MORE_CHARS(scanner)) {
        return 0;
    }

    if(!scanner->input.eof_read && scanner_flush(scanner, 0) < 0) {
        return -1;
    }

    next = *(scanner->buffer.next_char);
    if('\n' == next) {
        ++(scanner->input.line);
        scanner->input.column = 0;
    }

    ++(scanner->buffer.next_char);

    return next;
}

/**
 * Look at one of the next/previous characters in the input buffer. Returns
 * EOF if trying to look beyond the end of the file, 0 if trying to look beyond
 * either end of the buffer.
 */
char scanner_look(PScanner *scanner, const int n) {
    unsigned char *ch = scanner->buffer.next_char + (n - 1);

    if(ch >= scanner->buffer.end) {
        if(scanner->input.eof_read) {
            return EOF;
        }
        return 0;
    } else if(ch < scanner->buffer.start) {
        return 0;
    }

    return *ch;
}

/**
 * Push back n characters of input from the buffer. Intuitively, when trying
 * to match a lexeme against a pattern we might need to backtrack and undo a
 * decision. Such a reversal would require un-looking at characters.
 *
 * Returns 1 if all n characters were pushed back, 0 otherwise.
 */
int scanner_pushback(PScanner *scanner, int n) {

    unsigned char *curr_lexeme_start = scanner->lexeme.start,
                  *next_char = scanner->buffer.next_char;

    unsigned int new_line = scanner->input.line,
                 new_column;

    /* backtrack in the buffer */
    while(--n >= 0 && next_char > curr_lexeme_start) {
        --next_char;

        if('\n' == *next_char) {
            --new_line;
        } else if('\0' == *next_char) {
            break;
        }
    }

    /* figure out what column we're looking at on whatever line we have ended
     * up on. */
    if(new_line == scanner->lexeme.line) {
        new_column = (next_char - curr_lexeme_start)
                   + scanner->lexeme.column;

    } else if(new_line == scanner->input.line) {
        new_column = scanner->input.column
                   - (scanner->buffer.next_char - next_char);
    } else {
        /* the annoying case: we are *somewhere*, and we don't know where. what
         * we do know is that we are in-between where we have pushed back to and
         * the start of the current lexeme. we are neither on the same line as
         * where we were, nor are we on the same line as where the lexeme
         * started and so we just need to find the next '\n'.
         */
        for(new_column = 0; '\n' != *(next_char - new_column); ++new_column)
            ;
    }

    scanner->buffer.next_char = next_char;
    scanner->input.line = new_line;
    scanner->input.column = new_column;

    if(next_char < scanner->lexeme.end) {
        scanner->lexeme.end = next_char;
    }

    return (-1 == n);
}

/**
 * Instruct the scanner to skip characters up until the predicate fails.
 */
void scanner_skip(PScanner *scanner, PScannerSkipFunc *predicate) {
    assert_not_null(scanner);
    while(predicate(scanner_look(scanner, 1))) {
        scanner_advance(scanner);
    }
}

/**
 * Mark where the current lexeme begins and return its start position.
 */
unsigned char *scanner_mark_lexeme_start(PScanner *scanner) {
    unsigned char *start = scanner->buffer.next_char;
    scanner->lexeme.start = start;
    scanner->lexeme.end = start;
    scanner->lexeme.line = scanner->input.line;
    scanner->lexeme.column = scanner->input.column;
    scanner->lexeme.as_string = NULL;
    return start;
}

/**
 * Mark where the current lexeme ends and return its end position.
 */
void scanner_mark_lexeme_end(PScanner *scanner) {
    scanner->lexeme.end = scanner->buffer.next_char;
}

/**
 * Return the current lexeme as a PString. If the lexeme is empty (or no end
 * was marked for the lexeme) then NULL is returned. If the scanner is at the
 * end of input then NULL is returned.
 */
PString *scanner_get_lexeme(PScanner *scanner) {

    assert_not_null(scanner);

    if(is_not_null(scanner->lexeme.as_string)) {
        return scanner->lexeme.as_string;
    } else if(scanner->lexeme.end > scanner->lexeme.start) {
        return scanner->lexeme.as_string = string_alloc_char(
            (char *) scanner->lexeme.start,
            (uint32_t) (scanner->lexeme.end - scanner->lexeme.start - 1)
        );
    }

    return NULL;
}

