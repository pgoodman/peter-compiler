/*
 * headeer.h
 *
 *  Created on: May 10, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef HEADEER_H_
#define HEADEER_H_

#include "std-include.h"
#include "std-string.h"

/**
 * Wrapper around a C file handle.
 */
typedef struct PFileInputStream {
    FILE *ptr;
    char buff[4096];
} PFileInputStream;

PFileInputStream *file_read(const PString * const);
PFileInputStream *file_read_char(const char * const);
void file_free(PFileInputStream * );
unsigned char file_get_char(PFileInputStream *);

#endif /* HEADEER_H_ */
