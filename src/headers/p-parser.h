/*
 * p-parser.h
 *
 *  Created on: May 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PPARSER_H_
#define PPARSER_H_

#include <stdarg.h>
#include <unistd.h>

#include "std-include.h"
#include "std-input.h"
#include "adt-list.h"
#include "adt-dict.h"
#include "func-delegate.h"

#include "p-common-types.h"
#include "p-types.h"
#include "p-grammar-internal.h"

#include "p-parse-tree.h"

PParseTree *parse_tokens(PGrammar *grammar,
                         PToken tokens[],
                         int num_tokens);

#endif /* PPARSER_H_ */
