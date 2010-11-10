
type : '[A-Z][A-Za-z0-9_\-]*' ;
unbound_id : "_" ;
id : '[a-z*_\-!?<=>][a-zA-Z0-9*_\-!?<=>]*' ;
number : '[0-9]+(\.[0-9]+)?' ;
bool_true : "true" ;
bool_false : "false" ;

Program 
    : ^(-FuncDef Self | -TypeDef Self | <>)
    ;

    TypeDef
        : "type" -type ":" -(-TypeList "," ^Self | -TypeList) "."
        ;
        
        TypeList
            : -type -Self | -type
            ;
    
    FuncDef
        : "def" -id "(" -FuncParams ")" "->" -type ":" -StmtList
        ;

        FuncParams
            : -FuncParamList ^(";" ^FuncParams | <>)
            ;
        
        FuncParamList
            : -type ^(-id "," ^Self | -id)
            ;
   
Stmt
    : "with" -WithStmt
    | "bind" -BindStmt
    | "return" -ReturnStmt
    | "print" -PrintStmt
    | "pass" -PassStmt
    | Expr
    ;

    StmtList
        : -Stmt "," ^StmtList 
        | -Stmt "."
        ;
    
    WithStmt
        : -id "{" ^("bind" -BindStmt ^Self | "bind" -BindStmt) "}"
        ;
        
        BindStmt
            : -DestructureStmt "{" -StmtList "}"
            ;
            
            DestructureBind
                : -id
                | -unbound_id
                | -DestructureStmt
                ;
            
            DestructureStmt
                : -type "(" ^(^DestructureBind ^Self | ^DestructureBind) ")"
                ;
    
    ReturnStmt
        : ^Expr ("." ! >< | <>)
        ;
    
    PrintStmt
        : ^Expr
        ;
    
    PassStmt
        : <>
        ;
        
    Expr
        : id
        | number
        | bool_true
        | bool_false
        | "(" id ^(-Expr ^Self | <>) ")"
        ;
