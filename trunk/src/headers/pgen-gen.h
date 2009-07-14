/*
 * pgen-gen.h
 *
 *  Created on: Jul 11, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PGENGEN_H_
#define PGENGEN_H_

#include <string.h>
#include <ctype.h>

#include "std-include.h"
#include "std-string.h"

#include "adt-nfa.h"

#include "p-scanner.h"
#include "p-grammar.h"
#include "p-regexp.h"

#include "pgen-grammar.h"
#include "pgen-lexer.h"

void parser_gen(char *grammar_input_file,
                char *grammar_func_name,
                char *grammar_output_file,
                char *lexer_func_name,
                char *lexer_output_file,
                char *language_name);

#endif /* PGENGEN_H_ */
