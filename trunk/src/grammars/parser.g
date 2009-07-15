
non_terminal : '[A-Z][a-zA-Z0-9_]*' ;
terminal : '[a-z][a-zA-Z0-9_]*' ;
epsilon : "<>" ;
regexp : '\'([^\']*\\\')*[^\']*\'' ;
string : '"([^"]*\\")*[^"]*"' ;
cut : "!" ;
fail : "><" ;
non_excludable : "-" ;
raise_children : "^" ;
self : "Self" ;

GrammarRules
    : -Production ^GrammarRules
    | -Terminal ^GrammarRules
    | <>
    ;

Production
    : -non_terminal ":" ^ProductionRules ";"
    ;

Terminal
    : -terminal ":" ^(-regexp | -string) ";"
    ;

ProductionRules
    : -Rules "|" ^ProductionRules
    | -Rules
    ;

Rules
    : ^Rule ^Rules
    | ^Rule
    ;

Rule
    : -fail
    | -cut
    | ^RuleFlag "(" -ProductionRules ")"
    | ^RuleFlag ^(-self | -non_terminal | -regexp | -string | -terminal | -epsilon)
    ;

RuleFlag
    : -non_excludable
    | -raise_children
    | <>
    ;
