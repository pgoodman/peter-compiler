
GrammarRules
    : ^GrammarRules -Production
    : ^GrammarRules -Terminal
    : '\''
    ;

Production
    : -non_terminal ^ProductionRules ';'
    ;

Terminal : -terminal ':' -regexp ';' ;

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
