```
C-0文法

1.
<keyword> ::=
    const
    |int|void
    |if|else
    |while
    |main
    |return
    |printf|scanf

<additive operator> ::=
    +|－

<multiplicative operator> ::=
    *|/

<relation operator> ::=
    <|<=
    |>|>=
    |!=|==

<character> ::=
    _
    |a|...|z
    |A|...|Z

<number> ::=
    0|<nonzero number>

<nonzero number> ::=
    1|...|9

<special character> ::=
    \(|\)
    |\{|\}
    |,
    |;
    |=

<string> ::= "{<legal character>}"

2.
<program> ::=
    [<constant declarations>][<variable declarations>]{<function definitions>}<main function>

<constant declarations> ::=
    const <constant declaration>{,<constant declaration>};

<constant declaration> ::=
    <identifier>=<integer>

<integer> ::=
    [+|-]<nonzero number>{<number>}
	|0

<identifier> ::=
    <character>{<character>|<number>}

<声明头部> ::= int <identifier>

<variable declarations> ::=
    <声明头部>{,<identifier>};

<function definitions> ::=
    (<声明头部>|void <identifier>)<parameter><block>

<block> ::=
     \{[<constant declarations>][<variable declarations>]<statements>\}

<parameter> ::=
    \([<parameter list>]\)

<parameter list> ::=
    int <identifier>{,int <identifier>}

<main function> ::=
    (void|int) main<parameter><block>

<expression> ::=
    [+|-]<term>{<additive operator><term>}

<term> ::=
    <factor>{<multiplicative operator><factor>}

<factor> ::=
    <identifier>
    |\(<expression>\)
    |<integer>
    |<function call statement>

<statement> ::=
    <conditional statement>
    |<loop statement>
    |\{<statements>\}
    |<function call statement>;
    |<assignment statement>;
    |<return statement>;
    |<reading statement>;
    |<writing statement>;
    |<空>

<assignment statement> ::=
    <identifier>＝<expression>

<conditional statement> ::=
    if\(<condition>\)<statement>[else<statement>]

<condition> ::=
    <expression><relation operator><expression>
    |<expression>

<loop statement> ::=
    while\(<condition>\)<statement>

<function call statement> ::=
    <identifier>\([<valued parameter list>]\)

<valued parameter list> ::=
    <expression>{,<expression>}

<statements> ::=
    <statement>{<statement>}

<reading statement> ::=
    scanf\(<identifier>\)

<writing statement> ::=
    printf\([<string>,][<expression>]\)

<return statement> ::=
    return [\(<expression>\)]
```

