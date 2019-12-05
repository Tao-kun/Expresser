#ifndef EXPRESSER_FMP_HPP
#define EXPRESSER_FMP_HPP

#include <string>

#include "fmt/core.h"
#include "Types.h"
#include "Error/Error.h"
#include "Lexer/Token.h"

namespace fmt {
    template<>
    struct formatter<expresser::ExpresserError> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const expresser::ExpresserError &p, FormatContext &ctx) {
            return format_to(ctx.out(), "Line: {} Column: {} Error: {}",
                             p.GetPos().first, p.GetPos().second, p.GetCode());
        }
    };

    template<>
    struct formatter<expresser::ErrorCode> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const expresser::ErrorCode &p, FormatContext &ctx) {
            std::string name;
            switch (p) {
                case expresser::ErrAssignToConstant:
                    name = "AssignToConstant";
                    break;
                case expresser::ErrConstantNeedValue:
                    name = "ConstantNeedValue";
                    break;
                case expresser::ErrDoubleOverflow:
                    name = "DoubleOverflow";
                    break;
                case expresser::ErrDuplicateDeclaration:
                    name = "DuplicateDeclaration";
                    break;
                case expresser::ErrEOF:
                    name = "EOF";
                    break;
                case expresser::ErrIncompleteExpression:
                    name = "IncompleteExpression";
                    break;
                case expresser::ErrIncompleteFunction:
                    name = "IncompleteFunction";
                    break;
                case expresser::ErrInvalidAssignment:
                    name = "InvalidAssignment";
                    break;
                case expresser::ErrInvalidCharacter:
                    name = "InvalidCharacter";
                    break;
                case expresser::ErrInvalidDouble:
                    name = "InvalidDouble";
                    break;
                case expresser::ErrInvalidFunctionDeclaration:
                    name = "InvalidFunctionDeclaration";
                    break;
                case expresser::ErrInvalidIdentifier:
                    name = "InvalidIdentifier";
                    break;
                case expresser::ErrInvalidInput:
                    name = "InvalidInput";
                    break;
                case expresser::ErrInvalidInteger:
                    name = "InvalidInteger";
                    break;
                case expresser::ErrInvalidNotEqual:
                    name = "InvalidNotEqual";
                    break;
                case expresser::ErrInvalidPrint:
                    name = "InvalidPrint";
                    break;
                case expresser::ErrInvalidScan:
                    name = "InvalidScan";
                    break;
                case expresser::ErrInvalidStringLiteral:
                    name = "InvalidStringLiteral";
                    break;
                case expresser::ErrInvalidVariableDeclaration:
                    name = "InvalidVariableDeclaration";
                    break;
                case expresser::ErrIntegerOverflow:
                    name = "IntegerOverflow";
                    break;
                case expresser::ErrNeedIdentifier:
                    name = "NeedIdentifier";
                    break;
                case expresser::ErrNoSemicolon:
                    name = "NoSemicolon";
                    break;
                case expresser::ErrNotInitialized:
                    name = "NotInitialized";
                    break;
                case expresser::ErrNotDeclared:
                    name = "NotDeclared";
                    break;
                case expresser::ErrStreamError:
                    name = "StreamError";
                    break;
                case expresser::ErrUndeclaredFunction:
                    name = "UndeclaredFunction";
                    break;
                case expresser::ErrUndeclaredIdentifier:
                    name = "UndeclaredIdentifier";
                    break;
            }
            return format_to(ctx.out(), name);
        }
    };
}

namespace fmt {
    template<>
    struct formatter<expresser::Token> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(expresser::Token p, FormatContext &ctx) {
            return format_to(ctx.out(), "Line: {} Column: {} Type: {} Value: {}",
                             p.GetStartPos().first, p.GetStartPos().second, p.GetType(), p.GetStringValue());
        }
    };

    template<>
    struct formatter<expresser::TokenType> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const expresser::TokenType &p, FormatContext &ctx) {
            std::string name;
            switch (p) {
                case expresser::STRINGLITERAL:
                    name = "STRINGLITERAL";
                    break;
                case expresser::IDENTIFIER:
                    name = "IDENTIFIER";
                    break;
                case expresser::INTEGER:
                    name = "INTEGER";
                    break;
                case expresser::CHARLITERAL:
                    name = "CHARLITERAL";
                    break;
                case expresser::DOUBLE:
                    name = "DOUBLE";
                    break;
                case expresser::RESERVED:
                    name = "RESERVED";
                    break;
                case expresser::PLUS:
                    name = "PLUS";
                    break;
                case expresser::MINUS:
                    name = "MINUS";
                    break;
                case expresser::MULTIPLE:
                    name = "MULTIPLE";
                    break;
                case expresser::DIVIDE:
                    name = "DIVIDE";
                    break;
                case expresser::LESS:
                    name = "LESS";
                    break;
                case expresser::GREATER:
                    name = "GREATER";
                    break;
                case expresser::LESSEQUAL:
                    name = "LESSEQUAL";
                    break;
                case expresser::GREATEREQUAL:
                    name = "GREATEREQUAL";
                    break;
                case expresser::EQUAL:
                    name = "EQUAL";
                    break;
                case expresser::NOTEQUAL:
                    name = "NOTEQUAL";
                    break;
                case expresser::LEFTBRACKET:
                    name = "LEFTBRACKET";
                    break;
                case expresser::RIGHTBRACKET:
                    name = "RIGHTBRACKET";
                    break;
                case expresser::LEFTBRACE:
                    name = "LEFTBRACE";
                    break;
                case expresser::RIGHTBRACE:
                    name = "RIGHTBRACE";
                    break;
                case expresser::ASSIGN:
                    name = "ASSIGN";
                    break;
                case expresser::SEMICOLON:
                    name = "SEMICOLON";
                    break;
                case expresser::COLON:
                    name = "COLON";
                    break;
                case expresser::COMMA:
                    name = "COMMA";
                    break;
            }
            return format_to(ctx.out(), name);
        }
    };

}

#endif //EXPRESSER_FMP_HPP
