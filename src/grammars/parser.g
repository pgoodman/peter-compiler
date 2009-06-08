
Productions
    : ^Temp Production
    : <>
    ;

Temp : ^Productions ;

Production
    : -<non_terminal> ^ProductionRules <semicolon>
    ;

ProductionRules
    : -ProductionRule ^ProductionRules
    : <>
    ;

ProductionRule
    : <colon> ^Rules Decoration
    ;

Rules
    : ^Rule ^Rules
    : <>
    ;

Rule
    : RuleFlag -<non_terminal>
    : RuleFlag -<terminal>
    : RuleFlag -<epsilon>
    ;

Decoration
    : -<code>
    : <>
    ;

RuleFlag
    : -NonExcludable
    : -Subsumable
    : <>
    ;

NonExcludable : <dash> ;
Subsumable : <up_arrow> ;
