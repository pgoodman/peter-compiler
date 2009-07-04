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
#include "std-string.h"
#include "p-types.h"

PScanner *scanner_alloc(void);

void scanner_free(PScanner *scanner);

int scanner_open(PScanner *scanner, const char *file_name);

int scanner_flush(PScanner *scanner, int force_flush);

char scanner_advance(PScanner *scanner);

char scanner_look(PScanner *scanner, const int n);

int scanner_pushback(PScanner *scanner, int n);

void scanner_skip(PScanner *scanner, PScannerSkipFunc *predicate);

unsigned char *scanner_mark_lexeme_start(PScanner *scanner);

void scanner_mark_lexeme_end(PScanner *scanner);

PString *scanner_get_lexeme(PScanner *scanner);

#endif /* PSCANNER_H_ */
