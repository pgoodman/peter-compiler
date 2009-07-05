
Productions
    : ^Temp Production
    : ''
    ;

Temp
    : ^Productions
    ;

Production
    : -non_terminal ^ProductionRules semicolon
    ;

ProductionRules
    : -ProductionRule ^ProductionRules
    : ''
    ;

ProductionRule
    : colon ^Rules
    ;

Rules
    : ^Rule ^Rules
    : ''
    ;

Rule
    : RuleFlag -non_terminal
    : RuleFlag -terminal
    : RuleFlag -epsilon
    ;

RuleFlag
    : -NonExcludable
    : -Subsumable
    : ''
    ;

NonExcludable : dash ;
Subsumable : up_arrow ;
