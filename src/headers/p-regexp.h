/*
 * p-regexp.h
 *
 *  Created on: Jun 22, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PREGEXP_H_
#define PREGEXP_H_

#include <stdio.h>
#include <ctype.h>

#include <p-grammar.h>
#include <p-parser.h>
#include <p-scanner.h>

PParseTree *parse_regexp(const char *file);

#endif /* PREGEXP_H_ */
