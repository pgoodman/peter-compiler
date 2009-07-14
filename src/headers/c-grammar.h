/*
 * c-grammar.h
 *
 *  Created on: Jul 12, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef _PGEN_lang_grammar_
#define _PGEN_lang_grammar_

#include <p-scanner.h>
#include <p-grammar.h>
#include <p-parser.h>

enum {
    L_lang_string_6=0,
    L_lang_string_5=1,
    L_lang_string_4=2,
    L_lang_string_3=3,
    L_lang_string_2=4,
    L_lang_type_name=5,
    L_lang_string_1=6,
    L_lang_integer=7,
    L_lang_identifier=8,
    L_lang_string=9
};

enum {
    P_lang_IdentifierList=0,
    P_lang_TypedParameter=1,
    P_lang_Program=2,
    P_lang_FunctionDefinition=3,
    P_lang_TypedParameterList=4,
    P_lang_FunctionApplication=5,
    P_lang_TypeDestructure=6,
    P_lang_StatementList=7,
    P_lang_TypedIdentifier=8,
    P_lang_FunctionHeader=9,
    P_lang_Atom=10,
    P_lang_ExpressionList=11,
    P_lang_Type=12,
    P_lang_TypeList=13,
    P_lang_FunctionType=14,
    P_lang_Expression=15
};

PGrammar *lang_grammar(void);

#endif /* _PGEN_lang_grammar_ */
