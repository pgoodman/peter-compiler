
non_terminal : '[A-Z][a-zA-Z0-9_]*' ;
terminal : '[a-z][a-zA-Z0-9_]*' ;
epsilon : '\'\'' ;
regexp : '\'([^\']*\\\')*[^\']*\'' ;
cut : '!' ;
fail : '><' ;

GrammarRules
    : ^GrammarRules -Production
    : ^GrammarRules -Terminal
    : ''
    ;

Production : -non_terminal ^ProductionRules ';' ;

Terminal : -terminal ':' -regexp ';' ;

ProductionRules
    : -ProductionRule ^ProductionRules
    : ''
    ;

ProductionRule : ':' ^Rules ;

Rules
    : ^Rule ^Rules
    : ''
    ;

Rule
    : -fail
    : RuleFlag -non_terminal
    : RuleFlag -regexp
    : RuleFlag -terminal
    : RuleFlag -epsilon
    : -cut
    ;

RuleFlag
    : -NonExcludable
    : -RaiseChildren
    : ''
    ;

NonExcludable : '-' ;
RaiseChildren : '^' ;
