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
#include "adt-tree.h"
#include "adt-list.h"
#include "adt-dict.h"
#include "func-delegate.h"
#include "p-lexer.h"

#define $epsilon parser_rewrite_epsilon($A)
#define $prod(func) parser_rewrite_function($$A (func))
#define $tok(tok) parser_rewrite_token($$A (tok))

/* base parse types, holds our rewrite rules. */
typedef struct PParser {
    /*PGenericList *parsing_grammar;*/
    PDictionary *thunk_table;

    char is_closed; /* on for when we no longer allow rules to be added. */

    PDictionary *productions; /* keep track of all of the productions for the
                               * parsing grammar. */

    PDictionary *rules; /* keep track of rules used in productions, this does
                         * not keep track of epsilon rules as that is statically
                         * defined anyway. */
} PParser;

/* a full list of all of the tokens in a file. */
typedef struct PParserTokenList {
    PList _;
    PToken token;
} PParserTokenList;

/* a parse tree, constructed by the parser automatically */
typedef struct PParseTree {
    PTree _;
} PParseTree;

/* a parser function. a parser function deals with the *semantic* meaning of
 * a particular node in a parse tree. These functions are called *after* the
 * entire and correct parse tree is generated.
 */
typedef PParseTree *(*PParserFunc)($$ PParseTree *);

/* types relating to how the internal call stack is re-written using these
 * rules.
 */
typedef struct PParserRewriteRule {
    PParserFunc func;
    PLexeme lexeme;
} PParserRewriteRule;

PParser *parser_alloc($);
void parser_add_production($$ PParser *, PParserFunc, short, PParserRuleSequence *, ...);
PGenericList *parser_rule_sequence($$ short, PParserRewriteRule *, ...);
PParserRewriteRule *parser_rewrite_function($$ PParser *, PParserFunc);
PParserRewriteRule *parser_rewrite_token($$ PParser *, PLexeme);
PParserRewriteRule *parser_rewrite_epsilon($$ PParser *);

#endif /* PPARSER_H_ */
