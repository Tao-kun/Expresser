#ifndef EXPRESSER_LEXER_H
#define EXPRESSER_LEXER_H

#include <iostream>
#include <optional>
#include <vector>

#include "Lexer/Token.h"
#include "Error/Error.h"

namespace expresser {
    class Lexer final {
    private:
        std::istream &_input;
        bool _is_initialized;
        position_t _next_char_position;
        std::vector<std::string> _content_lines;

    public:
        explicit Lexer(std::istream &input);
        std::pair<std::optional<Token>, std::optional<ExpresserError>> NextToken();
        std::pair<std::vector<Token>, std::optional<ExpresserError>> AllTokens();
    private:
        void readAll();
        std::pair<std::optional<Token>, std::optional<ExpresserError>> nextToken();
        std::optional<ExpresserError> checkToken(const Token &t);
        position_t prevPos();
        position_t currPos();
        position_t nextPos();
        std::optional<char> nextChar();
        std::optional<char> peekNext();
        void rollback();
        bool isEOF();
        template<typename T>
        std::pair<std::optional<Token>, std::optional<ExpresserError>> makeToken(T value, position_t pos);

        enum DFAState {
            INITIAL_STATE,
            INTEGER_STATE,
            HEX_STATE,
            DOUBLE_STATE,
            PLUS_SIGN_STATE,
            MINUS_SIGN_STATE,
            MULTIPLICATION_SIGN_STATE,
            IDENTIFIER_STATE,
            SEMICOLON_STATE,
            COLON_STATE,
            COMMA_STATE,
            LEFTBRACKET_STATE,
            RIGHTBRACKET_STATE,
            LEFTBRACE_STATE,
            RIGHTBRACE_STATE,
            COMMENT_AND_DIVISION_SIGN_STATE,
            LESS_STATE,
            GREATER_STATE,
            ASSIGN_EQUAL_STATE,
            NOTEQUAL_STATE,
            CHAR_STATE,
            STRING_STATE
        };
    };
}

#endif //EXPRESSER_LEXER_H
