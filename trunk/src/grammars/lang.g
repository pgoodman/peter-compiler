
type_name : '[A-Z][a-zA-Z0-9_\-]*' ;
identifier : '[a-z\-_*+?$!>:/.<=^#][a-zA-Z0-9\-_*+?$!>:/.<=^#]*' ;
integer : '[0-9]+' ;
string : '"([^"]*\\")*[^"]*"' ;

Program
    : ^StatementList
    ;

FunctionType
    : "(" ^TypeList "->" -Type ")"
    ;

Type
    : -type_name
    | -FunctionType
    ;

TypeList
    : -Type ^TypeList
    | <>
    ;

IdentifierList
    : -identifier ^IdentifierList
    | <>
    ;

TypedIdentifier
    : -Type -identifier
    ;

TypeDestructure
    : -TypedIdentifier ^IdentifierList
    | -TypedIdentifier
    ;

TypedParameter
    : ^TypedIdentifier
    | "(" ^TypeDestructure ")"
    ;

TypedParameterList
    : -TypedParameter ^TypedParameterList
    | <>
    ;

FunctionHeader
    : "|" -TypedParameterList "->" -Type "|"
    ;

Atom
    : -integer
    | -string
    | -identifier
    ;

FunctionApplication
    : "(" -identifier ^ExpressionList ")"
    ;

Expression
    : ^Atom
    | -FunctionApplication
    ;

ExpressionList
    : Expression ExpressionList
    | <>
    ;

StatementList
    : Expression StatementList
    | FunctionDefinition StatementList
    | <>
    ;

FunctionDefinition
    : "(" "defun" -identifier -FunctionHeader -StatementList ")"
    ;
