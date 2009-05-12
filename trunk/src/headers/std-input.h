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

/**
 * Wrapper around a C file handle.
 */
typedef struct PFile {
    FILE _;
} PFile;

PFile *file_alloc($$ const char * const, const char * const );
void file_free($$ PFile * );

#endif /* HEADEER_H_ */
