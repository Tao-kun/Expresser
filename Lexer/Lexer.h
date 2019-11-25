#ifndef EXPRESSER_LEXER_H
#define EXPRESSER_LEXER_H

#include <iostream>
#include <optional>
#include <vector>

#include "Lexer/Token.h"
#include "Util/Error.h"

namespace expresser {
    class Lexer {
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
        position_t prevPos();
        position_t currPos();
        position_t nextPos();
        char nextChar();
        char peerNext();
        void rollback();
        bool isEOF();

    };
}

#endif //EXPRESSER_LEXER_H
