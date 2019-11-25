#ifndef EXPRESSER_TOKEN_H
#define EXPRESSER_TOKEN_H

#include <utility>
#include <variant>
#include <set>

namespace expresser {
    // 提前列出，待文法放出后删除多余部分
    enum TokenType : char {
        STRINGLITERAL, /* \"(\\.|[^"\\])*\" */
        IDENTIFIER, /* [a-zA-Z_]\w* 或 (?!\d)\w+ */
        INTEGER, /* \d+ */
        CHARLITERAL, /* \'[\x00-\xFF]\' */
        FLOAT,
        DOUBLE,

        RESERVED,

        PLUS, /* ARITHMETIC */
        MINUS,
        MULTIPLE,
        DIVIDE,
        MOD,

        NOT, /* BIT ARITHMETIC */
        AND,
        OR,
        XOR,
        SHR,
        SHL,

        LOGICALNOT,  /* LOGICAL ARITHMETIC */
        LOGICALAND,
        LOGICALOR,

        INC, /* INC DEC */
        DEC,

        LESS, /* RELATION */
        GREATER,
        LESSEQUAL,
        GREATEREQUAL,
        EQUAL,
        NOTEQUAL,

        LEFTBRACKET, /* BRACKETS */
        RIGHTBRACKET,
        LEFTPRA,
        RIGHTPRA,
        LEFTBRACE,
        RIGHTBRACE,

        ASSIGN, /* OTHER*/
        SEMICOLON,
        COMMA,
    };

    const static std::set<std::string> reserved_map{
            "const",
            "void", "int", "char", "double",
            "struct",
            "if", "else",
            "switch", "case", "default",
            "while", "for", "do",
            "return", "break", "continue",
            "print", "scan"
    };

    typedef std::variant<std::string, char, int32_t> token_value_t;
    typedef std::pair<uint32_t, uint32_t> position_t;

    class Token final {
    private:
        TokenType _type;
        position_t _start_pos;
        position_t _end_pos;
        token_value_t _value;
    public:
        TokenType GetType() const {
            return _type;
        }

        token_value_t GetValue() const {
            return _value;
        }

        std::string GetStringValue() {
            return std::get<std::string>(_value);
        }

        std::pair<uint32_t, uint32_t> GetStartPos() {
            return _start_pos;
        }

        std::pair<uint32_t, uint32_t> GetEndPos() {
            return _end_pos;
        }

        bool operator==(const Token &rhs) const {
            return _type == rhs._type &&
                   _start_pos == rhs._start_pos &&
                   _end_pos == rhs._end_pos &&
                   _value == rhs._value;
        }

        bool operator!=(const Token &rhs) const {
            return !(rhs == *this);
        }

    public:
        template<typename T>
        Token(TokenType type, T value, position_t start_pos, position_t end_pos) :
                _type(type), _start_pos(std::move(start_pos)), _end_pos(std::move(end_pos)), _value(value) {}

        template<typename T>
        Token(TokenType type, T value, uint32_t start_line, uint32_t start_line_pos,
              uint32_t end_line, uint32_t end_line_pos) :
                _type(type), _start_pos(position_t({start_line, start_line_pos})),
                _end_pos(position_t({end_line, end_line_pos})), _value(value) {}
    };
}

#endif //EXPRESSER_TOKEN_H
