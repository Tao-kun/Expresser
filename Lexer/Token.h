#ifndef EXPRESSER_TOKEN_H
#define EXPRESSER_TOKEN_H

namespace expresser {
    // 提前列出，待文法放出后删除多余部分
    enum TokenType : char {
        CHAR,
        INTEGER,
        STRING,
        IDENTIFIER,
        KEYWORD,
        PLUS,
        MINUS,
        MULTIPLE,
        DIVIDE,
        MOD,
        LESS,
        GREATER,
        LESSEQUAL,
        GREATEREQUAL,
        EQUAL,
        NOTEQUAL,
        ASSIGN,
        LEFTBRACKET,
        RIGHTBRACKET,
        LEFTPRA,
        RIGHTPRA,
        LEFTBRACE,
        RIGHTBRACE,
        SEMICOLON,
        COMMA,
        NOT,
        AND,
        OR,
        XOR,
        LOGICALNOT,
        LOGICALAND,
        LOGICALOR,
        SHR,
        SHL,
        INC,
        DEC
    };

    class Token {

    };
}

#endif //EXPRESSER_TOKEN_H
