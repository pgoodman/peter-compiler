/*
 * std-string.h
 *
 *  Created on: May 9, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef STDSTRING_H_
#define STDSTRING_H_

#include "vendor-pstdint.h"

typedef struct PString {
    uint16_t len;
    char *str;
} PString;

#endif /* STDSTRING_H_ */
