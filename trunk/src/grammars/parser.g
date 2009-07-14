
non_terminal : '[A-Z][a-zA-Z0-9_]*' ;
terminal : '[a-z][a-zA-Z0-9_]*' ;
epsilon : '<>' ;
regexp : '\'([^\']*\\\')*[^\']*\'' ;
string : '"([^"]*\\")*[^"]*"' ;
cut : '!' ;
fail : '><' ;
kleene_closure : "*" ;
positive_closure : "+" ;
optional : "?" ;
followed_by : '&' ;
not_followed_by : '~' ;
non_excludable : '-' ;
raise_children : '^' ;

GrammarRules
    : -Production ^GrammarRules
    | -Terminal ^GrammarRules
    | <>
    ;

Production
    : -non_terminal ':' ^ProductionRules ';'
    ;

Terminal
    : -terminal ':' ^(-regexp | -string) ';'
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
    | ^RuleFlag ^(-non_terminal | -regexp | -string | -terminal | -epsilon)
    ;

RuleFlag
    : -non_excludable
    | -raise_children
    | -kleene_closure
    | -positive_closure
    | -optional
    | -followed_by
    | -not_followed_by
    | <>
    ;
