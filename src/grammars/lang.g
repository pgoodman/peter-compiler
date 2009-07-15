
type_name : '[A-Z][a-zA-Z0-9_]*' ;
identifier : '[a-z_:][a-zA-Z0-9_:?!]*' ;
integer : '[0-9]+' ;
string : '"([^"]*\\")*[^"]*"' ;

Program
    : -Statement ^Program
    | <>
    ;

Statement
    : FunctionDefinition
    | Expression
    ;

Type
    :
    ;
    
FunctionDefinition
    : -identifier "(" -FunctionParameterList ")" ":" -Type "{" ^Program "}"
    ;

Expression
    : "(" Expression ")"
    | FunctionExpression
    ;
    
FunctionExpression
    : "(" -FunctionParameterList ")" ":" -Type "{" ^Program "}"
    ;

FunctionParameterList
    :
    ;