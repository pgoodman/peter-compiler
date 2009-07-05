
Productions
    : Production ^Productions
    : ''
    ;

Production
    : -non_terminal ^ProductionRules semicolon
    ;

ProductionRules
    : -ProductionRule ^ProductionRules
    : ''
    ;

ProductionRule
    : ':' ^Rules
    ;

Rules
    : ^Rule ^Rules
    : ''
    ;

Rule
    : RuleFlag -non_terminal
    : RuleFlag -regexp
    : RuleFlag -terminal
    : RuleFlag -epsilon
    ;

RuleFlag
    : -NonExcludable
    : -RaiseChildren
    : ''
    ;

NonExcludable : '-' ;
RaiseChildren : '^' ;
