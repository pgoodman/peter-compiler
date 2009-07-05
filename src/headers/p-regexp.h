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

#include "p-grammar.h"
#include "p-parser.h"
#include "p-scanner.h"

#include "adt-set.h"
#include "adt-nfa.h"

void regexp_parse(PGrammar *grammar,
                  PScanner *scanner,
                  PNFA *nfa,
                  unsigned char *regexp,
                  unsigned int start_state,
                  G_Terminal terminal);

PGrammar *regexp_grammar(void);

#endif /* PREGEXP_H_ */
