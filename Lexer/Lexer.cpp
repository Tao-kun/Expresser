#include <iostream>
#include <optional>
#include <vector>

#include "Lexer/Lexer.h"

namespace expresser {
    Lexer::Lexer(std::istream &input) : _input(input), _is_initialized(false) {}

    std::pair<std::optional<Token>, std::optional<ExpresserError>> Lexer::NextToken() {
        return std::pair<std::optional<Token>, std::optional<ExpresserError>>();
    }

    std::pair<std::vector<Token>, std::optional<ExpresserError>> Lexer::AllTokens() {
        return std::pair<std::vector<Token>, std::optional<ExpresserError>>();
    }

    void Lexer::readAll() {
        if (_is_initialized)
            return;
        for (std::string tmp; std::getline(_input, tmp);)
            _content_lines.emplace_back(tmp + "\n");
        _is_initialized = true;
        _next_char_position = std::make_pair<uint32_t, uint32_t>(0, 0);
    }

    std::pair<std::optional<Token>, std::optional<ExpresserError>> Lexer::nextToken() {
        return std::pair<std::optional<Token>, std::optional<ExpresserError>>();
    }

    position_t Lexer::prevPos() {
        return expresser::position_t();
    }

    position_t Lexer::currPos() {
        return expresser::position_t();
    }

    position_t Lexer::nextPos() {
        return expresser::position_t();
    }

    bool Lexer::isEOF() {
        return false;
    }

    char Lexer::nextChar() {
        return 0;
    }

    char Lexer::peerNext() {
        return 0;
    }

    void Lexer::rollback() {

    }
}