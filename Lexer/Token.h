#ifndef EXPRESSER_TOKEN_H
#define EXPRESSER_TOKEN_H

#include <any>
#include <map>
#include <utility>
#include <variant>
#include <set>
#include <string>

#include "Types.h"
#include "Error/Error.h"

namespace expresser {
    // 提前列出，待文法放出后删除多余部分
    enum TokenType : char {
        STRINGLITERAL, /* \"(\\.|[^"\\])*\" */
        IDENTIFIER, /* [a-zA-Z_]\w* 或 (?!\d)\w+ */
        INTEGER, /* \d+ */
        CHARLITERAL, /* \'[\x00-\xFF]\' */
        DOUBLE,

        RESERVED,

        PLUS, /* ARITHMETIC */
        MINUS,
        MULTIPLE,
        DIVIDE,

        LESS, /* RELATION */
        GREATER,
        LESSEQUAL,
        GREATEREQUAL,
        EQUAL,
        NOTEQUAL,

        LEFTBRACKET, /* BRACKETS */
        RIGHTBRACKET,
        LEFTBRACE,
        RIGHTBRACE,

        ASSIGN, /* OTHER*/
        SEMICOLON,
        COLON,
        COMMA,
    };

    const static std::set<std::string> reserved_set{
            "const",
            "void", "int", "char", "double",
            "struct",
            "if", "else",
            "switch", "case", "default",
            "while", "for", "do",
            "return", "break", "continue",
            "print", "scan"
    };

    const static std::map<std::variant<char, std::string>, TokenType> simple_token_map{
            {'+',  PLUS},
            {'-',  MINUS},
            {'*',  MULTIPLE},
            {'(',  LEFTBRACKET},
            {')',  RIGHTBRACKET},
            {'{',  LEFTBRACE},
            {'}',  RIGHTBRACE},
            {';',  SEMICOLON},
            {':',  COLON},
            {',',  COMMA},
            {'<',  LESS},
            {'>',  GREATER},
            {'=',  ASSIGN},
            {"<=", LESSEQUAL},
            {">=", GREATEREQUAL},
            {"==", EQUAL}
    };

    class Token final {
    private:
        TokenType _type;
        position_t _start_pos;
        position_t _end_pos;
        std::any _value;
    public:
        TokenType GetType() const {
            return _type;
        }

        std::any GetValue() const {
            return _value;
        }

        std::string GetStringValue() const {
            try {
                return std::any_cast<std::string>(_value);
            }
            catch (const std::bad_any_cast &) {}
            try {
                return std::string(1, std::any_cast<char>(_value));
            }
            catch (const std::bad_any_cast &) {}
            try {
                return std::to_string(std::any_cast<int32_t>(_value));
            }
            catch (const std::bad_any_cast &) {}
            try {
                return std::to_string(std::any_cast<double>(_value));
            }
            catch (const std::bad_any_cast &) {
                ExceptionPrint("No suitable cast for token value.");
            }
            return "Invalid";
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
                   GetStringValue() == rhs.GetStringValue();
        }

        bool operator!=(const Token &rhs) const {
            return !(rhs == *this);
        }

    public:
        Token(TokenType type, std::any value, position_t start_pos, position_t end_pos) :
                _type(type), _start_pos(std::move(start_pos)), _end_pos(std::move(end_pos)), _value(std::move(value)) {}

        Token(TokenType type, std::any value, uint32_t start_line, uint32_t start_line_pos,
              uint32_t end_line, uint32_t end_line_pos) :
                _type(type), _start_pos(position_t({start_line, start_line_pos})),
                _end_pos(position_t({end_line, end_line_pos})), _value(std::move(value)) {}
    };
}

#endif //EXPRESSER_TOKEN_H
