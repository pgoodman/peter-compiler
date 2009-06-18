/*
 * p-lexer.h
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PSCANNER_H_
#define PSCANNER_H_

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "std-include.h"
#include "p-types.h"

PScanner *scanner_alloc(void);

void scanner_free(PScanner *scanner);

#endif /* PSCANNER_H_ */
