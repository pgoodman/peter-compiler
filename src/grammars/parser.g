
Grammar
    : Production Productionslef
    ;

Productions
    : Grammar
    : <>
    ;

Production
    : <non_terminal> ProductionRules <semicolon>
    ;

ProductionRules
    : ProductionRule ProductionRules
    : <>
    ;

ProductionRule
    : <colon> Rules Decoration
    ;

Rules
    : Rule Rules
    : <>
    ;

Rule
    : <non_terminal>
    : <terminal>
    : <epsilon>
    ;

Decoration
    : <code>
    : <>
    ;
