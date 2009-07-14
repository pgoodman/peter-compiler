/*
 * pgen-grammar.h
 *
 *  Created on: Jul 11, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef PGENGRAMMAR_H_
#define PGENGRAMMAR_H_

#include "p-scanner.h"
#include "p-grammar.h"
#include "p-parser.h"

enum {
    L_pg_string_5=0,
    L_pg_positive_closure=1,
    L_pg_epsilon=2,
    L_pg_string_4=3,
    L_pg_string_3=4,
    L_pg_followed_by=5,
    L_pg_regexp=6,
    L_pg_regexp_2=7,
    L_pg_non_terminal=8,
    L_pg_regexp_1=9,
    L_pg_kleene_closure=10,
    L_pg_non_excludable=11,
    L_pg_terminal=12,
    L_pg_cut=13,
    L_pg_raise_children=14,
    L_pg_not_followed_by=15,
    L_pg_string=16,
    L_pg_optional=17,
    L_pg_fail=18
};

enum {
    P_pg_RuleFlag=0,
    P_pg_ProductionRules=1,
    P_pg_Terminal=2,
    P_pg_Production=3,
    P_pg_Rule=4,
    P_pg_GrammarRules=5,
    P_pg_Rules=6,
    P_pg_subrule_1=7,
    P_pg_subrule_2=8
};

PGrammar *parser_grammar_grammar(void);



#endif /* PGENGRAMMAR_H_ */
