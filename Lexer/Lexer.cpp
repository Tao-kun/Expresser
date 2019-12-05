#include <optional>
#include <regex>
#include <vector>

#include "Lexer/Lexer.h"
#include "Lexer/Utils.hpp"

namespace expresser {
    Lexer::Lexer(std::istream &input) : _input(input), _is_initialized(false) {}

    std::pair<std::optional<Token>, std::optional<ExpresserError>> Lexer::NextToken() {
        if (!_is_initialized)
            readAll();
        if (_input.bad())
            return std::make_pair(std::optional<Token>(),
                                  std::make_optional<ExpresserError>(0, 0, ErrorCode::ErrStreamError));
        if (isEOF())
            return std::make_pair(std::optional<Token>(),
                                  std::make_optional<ExpresserError>(0, 0, ErrorCode::ErrEOF));
        auto p = nextToken();
        if (p.second.has_value())
            return std::make_pair(p.first, p.second);
        auto err = checkToken(p.first.value());
        if (err.has_value())
            return std::make_pair(p.first, err.value());
        return std::make_pair(p.first, std::optional<ExpresserError>());
    }
    std::pair<std::vector<Token>, std::optional<ExpresserError>> Lexer::AllTokens() {
        std::vector<Token> result;
        for (;;) {
            auto p = NextToken();
            if (p.second.has_value()) {
                if (p.second.value().GetCode() == ErrorCode::ErrEOF)
                    return std::make_pair(result, std::optional<ExpresserError>());
                else
                    return std::make_pair(std::vector<Token>(), p.second);
            }
            result.emplace_back(p.first.value());
        }
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
        std::stringstream ss;
        std::pair<std::optional<Token>, std::optional<ExpresserError>> result;
        position_t pos;
        DFAState current_state = INITIAL_STATE;

        // 辅助函数
        auto return_int = [&]() {
            int32_t int_value;
            ss >> int_value;
            if (ss.fail())
                return std::make_pair(std::optional<Token>(),
                                      std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidInteger));
            else
                return std::make_pair(std::make_optional<Token>(TokenType::INTEGER, int_value, pos, currPos()),
                                      std::optional<ExpresserError>());
        };
        auto return_ident = [&]() {
            std::string str;
            ss >> str;
            if (ss.fail())
                return std::make_pair(std::optional<Token>(),
                                      std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidIdentifier));;
            auto it = reserved_set.find(str);
            if (it != reserved_set.end())
                return std::make_pair(std::make_optional<Token>(RESERVED, it, pos, currPos()),
                                      std::optional<ExpresserError>());
            else
                return std::make_pair(std::make_optional<Token>(IDENTIFIER, it, pos, currPos()),
                                      std::optional<ExpresserError>());
        };
        auto return_double = [&]() {
            double double_literal;
            ss >> double_literal;
            if (ss.fail())
                return std::make_pair(std::optional<Token>(),
                                      std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidDouble));
            else
                return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, double_literal, pos, currPos()),
                                      std::optional<ExpresserError>());
        };

        for (;;) {
            auto current_char = nextChar();
            switch (current_state) {
                case INITIAL_STATE: {
                    if (!current_char.has_value())
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<ExpresserError>(currPos(), ErrorCode::ErrEOF));
                    auto ch = current_char.value();
                    auto invalid = false;

                    if (expresser::isspace(ch))
                        current_state = DFAState::INITIAL_STATE;
                    else if (!expresser::isprint(ch))
                        invalid = true;
                    else if (expresser::isdigit(ch))
                        current_state = DFAState::INTEGER_STATE;
                    else if (expresser::isalpha(ch))
                        current_state = DFAState::IDENTIFIER_STATE;
                    else {
                        // TODO: impl it
                        switch (ch) {
                            case '+':
                                current_state = PLUS_SIGN_STATE;
                                break;
                            case '-':
                                current_state = MINUS_SIGN_STATE;
                                break;
                            case '*':
                                current_state = MULTIPLICATION_SIGN_STATE;
                                break;
                            case '/':
                                current_state = COMMENT_AND_DIVISION_SIGN_STATE;
                                break;
                            case '<':
                                current_state = LESS_STATE;
                                break;
                            case '>':
                                current_state = GREATER_STATE;
                                break;
                            case '=':
                                current_state = ASSIGN_EQUAL_STATE;
                                break;
                            case '!':
                                current_state = NOTEQUAL_STATE;
                                break;
                            case '(':
                                current_state = LEFTBRACKET_STATE;
                                break;
                            case ')':
                                current_state = RIGHTBRACKET_STATE;
                                break;
                            case '{':
                                current_state = LEFTBRACE_STATE;
                                break;
                            case '}':
                                current_state = RIGHTBRACE_STATE;
                                break;
                            case ':':
                                current_state = COLON_STATE;
                                break;
                            case ';':
                                current_state = SEMICOLON_STATE;
                                break;
                            case '\'':
                                current_state = CHAR_STATE;
                                break;
                            case '\"':
                                current_state = STRING_STATE;
                                break;
                            case ',':
                                current_state = COMMA_STATE;
                                break;
                            default: {
                                invalid = true;
                                break;
                            }
                        }
                    }
                    if (current_state != DFAState::INITIAL_STATE)
                        pos = prevPos();
                    if (invalid) {
                        rollback();
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidInput));
                    }
                    if (current_state != DFAState::INITIAL_STATE)
                        ss << ch;
                    break;
                }
                case PLUS_SIGN_STATE:
                case MINUS_SIGN_STATE:
                case MULTIPLICATION_SIGN_STATE:
                case LEFTBRACKET_STATE:
                case RIGHTBRACKET_STATE:
                case LEFTBRACE_STATE:
                case RIGHTBRACE_STATE:
                case COLON_STATE:
                case SEMICOLON_STATE:
                case COMMA_STATE: {
                    // 都是单字符的token，所以回退一次返回即可
                    rollback();
                    char ch;
                    ss >> ch;
                    if (ss.fail())
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidInput));
                    auto it = simple_token_map.find(ch);
                    if (it != simple_token_map.end())
                        return std::make_pair(std::make_optional<Token>(it->second, it->first, pos, currPos()),
                                              std::optional<ExpresserError>());
                    else
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidCharacter));
                }
                case LESS_STATE:
                case GREATER_STATE:
                case ASSIGN_EQUAL_STATE: {
                    // 预读，如果下一个字符是'='就是LEQ/GEQ/ASSIGN
                    // 否则就是LESS/GREATER/EQUAL
                    char ch;
                    if (current_char.has_value()) {
                        ch = current_char.value();
                        if (ch == '=') {
                            ss << ch;
                            std::string str;
                            ss >> str;
                            if (ss.fail())
                                return std::make_pair(std::optional<Token>(),
                                                      std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidInput));
                            return makeToken<std::string>(str, pos);
                        }
                    }
                    // 下一个不是'='，或者下一个为EOF
                    ss >> ch;
                    if (ss.fail())
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidInput));
                    return makeToken<char>(ch, pos);
                }
                case NOTEQUAL_STATE: {
                    // 预读，如果不是'='就报错
                    if (current_char.has_value()) {
                        if (current_char.value() == '=') {
                            return std::make_pair(std::make_optional<Token>(TokenType::NOTEQUAL, std::string("!="), pos, currPos()),
                                                  std::optional<ExpresserError>());
                        }
                    }
                    return std::make_pair(std::optional<Token>(),
                                          std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidNotEqual));
                }
                case INTEGER_STATE: {
                    // TODO: impl it
                    // 普通 '0'|<nonzero-digit>{<digit>}
                    // 十六进制 碰到'x'/'X'就跳转 ('0x'|'0X')<hexadecimal-digit>{<hexadecimal-digit>}
                    // 浮点字面量 碰到'.'就跳转
                    // 其他报错
                    for (; current_state == INTEGER_STATE; current_char = nextChar()) {
                        if (!current_char.has_value()) {
                            return return_int();
                        }
                        char ch = current_char.value();
                        if (expresser::isdigit(ch)) {
                            ss << ch;
                        } else if (ch == 'x' || ch == 'X') {
                            ss << ch;
                            current_state = HEX_STATE;
                        } else if (ch == '.') {
                            ss << ch;
                            current_state = DOUBLE_STATE;
                        } else if (expresser::isalpha(ch)) {
                            ss << ch;
                            current_char = IDENTIFIER_STATE;
                        } else {
                            rollback();
                            return return_int();
                        }
                    }
                    break;
                }
                case HEX_STATE: {
                    // TODO: impl it
                    // 从INTEGER_STATE跳转而来
                    for (; current_state == HEX_STATE; current_char = nextChar()) {
                        if (!current_char.has_value()) {
                            return return_int();
                        }
                        char ch = current_char.value();
                        if (expresser::isxdigit(ch)) {
                            ss << ch;
                        } else if (expresser::isalpha(ch)) {
                            ss << ch;
                            current_char = IDENTIFIER_STATE;
                        } else {
                            rollback();
                            return return_int();
                        }
                    }
                    break;
                }
                case DOUBLE_STATE: {
                    // 从INTEGER_STATE跳转而来
                    // <sign> ::=
                    //    '+'|'-'
                    // <digit-seq> ::=
                    //    <digit>{<digit>}
                    // <floating-literal> ::=
                    //     [<digit-seq>]'.'<digit-seq>[<exponent>]
                    //    |<digit-seq>'.'[<exponent>]
                    //    |<digit-seq><exponent>
                    // <exponent> ::=
                    //    ('e'|'E')[<sign>]<digit-seq>
                    bool has_exponent = false;

                    for (;; current_char = nextChar()) {
                        if (!current_char.has_value()) {
                            return return_double();
                        }
                        char ch = current_char.value();
                        if (isdigit(ch) && !has_exponent) {
                            ss << ch;
                        } else if (ch == '.' || (has_exponent && (ch == 'e' || ch == 'E'))) {
                            // 只能有一个exponent，只能有一个小数点
                            return std::make_pair(std::optional<Token>(),
                                                  std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidDouble));
                        } else if (ch == 'e' || ch == 'E') {
                            ss << ch;
                            has_exponent = true;
                        } else {
                            rollback();
                            return return_double();
                        }
                    }
                    break;
                }
                case IDENTIFIER_STATE: {
                    for (;; current_char = nextChar()) {
                        if (!current_char.has_value())
                            return return_ident();
                        char ch = current_char.value();
                        if (expresser::isalpha(ch) || expresser::isdigit(ch))
                            ss << ch;
                        else {
                            rollback();
                            return return_ident();
                        }
                    }
                }
                case COMMENT_AND_DIVISION_SIGN_STATE: {
                    if (!current_char.has_value() || current_char.value() != '*' || current_char.value() != '/') {
                        rollback();
                        return std::make_pair(std::make_optional<Token>(TokenType::DIVIDE, '/', pos, currPos()),
                                              std::optional<ExpresserError>());
                    }
                    char ch = current_char.value();
                    if (ch == '*') {
                        // <multi-line-comment> ::=
                        //    '/*'{<any-char>}'*/'
                        for (; current_state == COMMENT_AND_DIVISION_SIGN_STATE; current_char = nextChar()) {
                            if (!current_char.has_value()) {
                                current_state = INITIAL_STATE;
                                break;
                            } else if (current_char.value() == '*') {
                                // 预读
                                current_char = nextChar();
                                if (current_char.has_value() && current_char.value() == '/') {
                                    // 匹配，返回初始状态
                                    current_state = INITIAL_STATE;
                                    break;
                                } else {
                                    // 不匹配，回滚
                                    rollback();
                                }
                            }
                        }
                    } else if (ch == '/') {
                        // <single-line-comment> ::=
                        //    '//'{<any-char>}<LF>
                        for (; current_state == COMMENT_AND_DIVISION_SIGN_STATE; current_char = nextChar()) {
                            if (!current_char.has_value() || current_char.value() == '\n') {
                                // EOF或者注释结束，返回初始状态
                                current_state = INITIAL_STATE;
                                break;
                            }
                        }
                    } else {
                        ExceptionPrint("Unhandled comment!\n");
                    }
                    break;
                }
                    /* TODO: impl it
                    case CHAR_STATE: {
                        break;
                    }
                    case STRING_STATE: {
                        break;
                    }
                    */
                default: {
                    ExceptionPrint("unhandled state");
                    break;
                }
            }
        }
        return std::make_pair(std::optional<Token>(),
                              std::optional<ExpresserError>());
    }
    std::optional<ExpresserError> Lexer::checkToken(const Token &t) {
        const static std::regex ident_regex("[a-zA-Z][0-9a-zA-Z]*", std::regex::extended);
        switch (t.GetType()) {
            case IDENTIFIER: {
                std::string val = t.GetStringValue();
                if (!std::regex_match(val, ident_regex))
                    return std::make_optional<ExpresserError>(currPos(), ErrorCode::ErrInvalidIdentifier);
                break;
            }
            default:
                break;
        }
        return {};
    }
    position_t Lexer::prevPos() {
        if (_next_char_position.first == 0 && _next_char_position.second == 0)
            ExceptionPrint("current position is file begining, no previous");
        if (_next_char_position.second == 0)
            return std::make_pair(_next_char_position.first - 1,
                                  _content_lines[_next_char_position.first - 1].size() - 1);
        return std::make_pair(_next_char_position.first, _next_char_position.second - 1);
    }

    position_t Lexer::currPos() {
        return _next_char_position;
    }

    position_t Lexer::nextPos() {
        if (isEOF() || _next_char_position.first >= _content_lines.size())
            ExceptionPrint("you are at end of this file");
        if (_next_char_position.second == _content_lines[_next_char_position.first].size() - 1)
            return std::make_pair(_next_char_position.first + 1, 0);
        else
            return std::make_pair(_next_char_position.first, _next_char_position.second + 1);
    }

    bool Lexer::isEOF() {
        return _next_char_position.first >= _content_lines.size();
    }

    std::optional<char> Lexer::nextChar() {
        if (isEOF())
            return {};
        _next_char_position = nextPos();
        return _content_lines[_next_char_position.first][_next_char_position.second];
    }

    std::optional<char> Lexer::peekNext() {
        if (isEOF())
            return {};
        position_t pos = nextPos();
        return _content_lines[pos.first][pos.second];
    }

    void Lexer::rollback() {
        _next_char_position = prevPos();
    }

    template<typename T>
    std::pair<std::optional<Token>, std::optional<ExpresserError>> Lexer::makeToken(T value, position_t pos) {
        auto it = simple_token_map.find(value);
        if (it != simple_token_map.end()) {
            return std::make_pair(std::make_optional<Token>(it->second, it->first, pos, currPos()),
                                  std::optional<ExpresserError>());
        } else
            return std::make_pair(std::optional<Token>(),
                                  std::make_optional<ExpresserError>(pos, ErrorCode::ErrInvalidInput));
    }
}