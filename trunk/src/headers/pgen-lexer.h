/*
 * pgen-lexer.h
 *
 *  Created on: Jul 11, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PGENLEXER_H_
#define PGENLEXER_H_

#include <ctype.h>
#include <stdio.h>

#include "std-include.h"
#include "p-types.h"
#include "p-scanner.h"

G_Terminal parser_grammar_lexer(PScanner *S);

#endif /* PGENLEXER_H_ */
