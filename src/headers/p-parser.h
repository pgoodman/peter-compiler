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

#include "p-prod-common.h"
#include "p-parser-types.h"
#include "p-adt.h"

PParseTree *parser_parse_tokens(PParser *, PTokenGenerator *);
void parser_free_parse_tree(PParseTree *tree);

#endif /* PPARSER_H_ */
