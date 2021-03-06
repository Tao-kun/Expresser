#ifndef EXPRESSER_FMP_HPP
#define EXPRESSER_FMP_HPP

#include <string>

#include "fmt/core.h"
#include "Types.h"
#include "Error/Error.h"
#include "Lexer/Token.h"
#include "Instruction/Instruction.h"

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
                case expresser::ErrCallFunctionInStartSection:
                    name = "Not support call a function in start section";
                    break;
                case expresser::ErrCastToVoid:
                    name = "CastToVoid";
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
                    name = "Incomplete Function";
                    break;
                case expresser::ErrInvalidAssignment:
                    name = "Invalid Assignment: only need semicolon, assign symbol, comma";
                    break;
                case expresser::ErrInvalidCast:
                    name = "InvalidCast: unknown cast type";
                    break;
                case expresser::ErrInvalidCharacter:
                    name = "InvalidCharacter";
                    break;
                case expresser::ErrInvalidDouble:
                    name = "InvalidDouble";
                    break;
                case expresser::ErrInvalidExpression:
                    name = "InvalidExpression: input not match:\n"
                           "<primary-expression> ::=\n"
                           "     '('<expression>')'\n"
                           "    |<identifier>\n"
                           "    |<integer-literal>\n"
                           "    |<function-call>\n"
                           "    |<char-literal>";
                    break;
                case expresser::ErrInvalidFunctionCall:
                    name = "InvalidFunctionCall: EOF before a <function-call> ends\n"
                           "<function-call> ::=\n"
                           "    <identifier> '(' [<expression-list>] ')'\n"
                           "<expression-list> ::=\n"
                           "    <expression>{','<expression>}";
                    break;
                case expresser::ErrInvalidFunctionReturnType:
                    name = "InvalidFunctionReturnType";
                    break;
                case expresser::ErrInvalidIdentifier:
                    name = "InvalidIdentifier: not match <identifier> ::= <nondigit>{<nondigit>|<digit>}";
                    break;
                case expresser::ErrInvalidInput:
                    name = "InvalidInput";
                    break;
                case expresser::ErrInvalidInteger:
                    name = "InvalidInteger";
                    break;
                case expresser::ErrInvalidJump:
                    name = "InvalidJump";
                    break;
                case expresser::ErrInvalidLoop:
                    name = "InvalidLoop";
                    break;
                case expresser::ErrInvalidNotEqual:
                    name = "InvalidNotEqual";
                    break;
                case expresser::ErrInvalidParameter:
                    name = "InvalidParameter";
                    break;
                case expresser::ErrInvalidPrint:
                    name = "InvalidPrint";
                    break;
                case expresser::ErrInvalidScan:
                    name = "InvalidScan";
                    break;
                case expresser::ErrInvalidStatement:
                    name = "InvalidStatement";
                    break;
                case expresser::ErrInvalidStringLiteral:
                    name = "InvalidStringLiteral";
                    break;
                case expresser::ErrInvalidVariableDeclaration:
                    name = "InvalidVariableDeclaration";
                    break;
                case expresser::ErrInvalidVariableType:
                    name = "InvalidVariableType";
                    break;
                case expresser::ErrIntegerOverflow:
                    name = "IntegerOverflow";
                    break;
                case expresser::ErrMissingBrace:
                    name = "MissingBrace";
                    break;
                case expresser::ErrMissingBracket:
                    name = "MissingBracket";
                    break;
                case expresser::ErrMissingRightQuote:
                    name = "MissingRightQuote";
                    break;
                case expresser::ErrNeedAssignSymbol:
                    name = "NeedAssignSymbol";
                    break;
                case expresser::ErrNeedFunctionName:
                    name = "Need a function name in <function-call>";
                    break;
                case expresser::ErrNeedIdentifier:
                    name = "NeedIdentifier";
                    break;
                case expresser::ErrNeedRelationalOperator:
                    name = "NeedRelationalOperator";
                    break;
                case expresser::ErrNeedSemicolon:
                    name = "NeedSemicolon";
                    break;
                case expresser::ErrNeedSemicolonOrComma:
                    name = "NeedSemicolonOrComma";
                    break;
                case expresser::ErrNeedVariableType:
                    name = "NeedVariableType";
                    break;
                case expresser::ErrNeedWhileInDoWhile:
                    name = "NeedWhileInDoWhile";
                    break;
                case expresser::ErrNotInitialized:
                    name = "NotInitialized";
                    break;
                case expresser::ErrNotDeclared:
                    name = "NotDeclared";
                    break;
                case expresser::ErrReturnInVoidFunction:
                    name = "ReturnInVoidFunction";
                    break;
                case expresser::ErrStreamError:
                    name = "StreamError";
                    break;
                case expresser::ErrUnknownEscapeCharacter:
                    name = "UnknownEscapeCharacter";
                    break;
                case expresser::ErrUndeclaredFunction:
                    name = "UndeclaredFunction";
                    break;
                case expresser::ErrUndeclaredIdentifier:
                    name = "UndeclaredIdentifier";
                    break;
                default:
                    name = "Unhandled Error";
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
                case expresser::VOID:
                    name = "VOID";
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

namespace fmt {
    template<>
    struct formatter<expresser::Instruction> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(expresser::Instruction inst, FormatContext &ctx) {
            auto params = inst.GetParams();
            std::string string_param;
            for (;;) {
                int32_t value;
                if (params.first._size == 0)
                    break;
                value = 0;
                ::memcpy(&value, params.first._value, params.first._size);
                string_param += " ";
                string_param += std::to_string(value);
                if (params.second._size == 0)
                    break;
                value = 0;
                ::memcpy(&value, params.second._value, params.second._size);
                string_param += "," + std::to_string(value);
                break;
            }
            return format_to(ctx.out(), "{} {}{}", std::to_string(inst.GetIndex()), inst.GetOperation(), string_param);
        }
    };

    template<>
    struct formatter<expresser::Operation> {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

        template<typename FormatContext>
        auto format(const expresser::Operation &op, FormatContext &ctx) {
            std::string name;
            switch (op) {
                case expresser::NOP:
                    name = "nop";
                    break;
                case expresser::BIPUSH:
                    name = "bipush";
                    break;
                case expresser::IPUSH:
                    name = "ipush";
                    break;
                case expresser::POP:
                    name = "pop";
                    break;
                case expresser::POP2:
                    name = "pop2";
                    break;
                case expresser::POPN:
                    name = "popn";
                    break;
                case expresser::DUP:
                    name = "dup";
                    break;
                case expresser::DUP2:
                    name = "dup2";
                    break;
                case expresser::LOADC:
                    name = "loadc";
                    break;
                case expresser::LOADA:
                    name = "loada";
                    break;
                case expresser::NEW:
                    name = "new";
                    break;
                case expresser::SNEW:
                    name = "snew";
                    break;
                case expresser::ILOAD:
                    name = "iload";
                    break;
                case expresser::DLOAD:
                    name = "dload";
                    break;
                case expresser::ALOAD:
                    name = "aload";
                    break;
                case expresser::IALOAD:
                    name = "iaload";
                    break;
                case expresser::DALOAD:
                    name = "daload";
                    break;
                case expresser::AALOAD:
                    name = "aaload";
                    break;
                case expresser::ISTORE:
                    name = "istore";
                    break;
                case expresser::DSTORE:
                    name = "dstore";
                    break;
                case expresser::ASTORE:
                    name = "astore";
                    break;
                case expresser::IASTORE:
                    name = "iastore";
                    break;
                case expresser::DASTORE:
                    name = "dastore";
                    break;
                case expresser::AASTORE:
                    name = "aastore";
                    break;
                case expresser::IADD:
                    name = "iadd";
                    break;
                case expresser::DADD:
                    name = "dadd";
                    break;
                case expresser::ISUB:
                    name = "isub";
                    break;
                case expresser::DSUB:
                    name = "dsub";
                    break;
                case expresser::IMUL:
                    name = "imul";
                    break;
                case expresser::DMUL:
                    name = "dmul";
                    break;
                case expresser::IDIV:
                    name = "idiv";
                    break;
                case expresser::DDIV:
                    name = "ddiv";
                    break;
                case expresser::INEG:
                    name = "ineg";
                    break;
                case expresser::DNEG:
                    name = "dneg";
                    break;
                case expresser::ICMP:
                    name = "icmp";
                    break;
                case expresser::DCMP:
                    name = "dcmp";
                    break;
                case expresser::I2D:
                    name = "i2d";
                    break;
                case expresser::D2I:
                    name = "d2i";
                    break;
                case expresser::I2C:
                    name = "i2c";
                    break;
                case expresser::JMP:
                    name = "jmp";
                    break;
                case expresser::JE:
                    name = "je";
                    break;
                case expresser::JNE:
                    name = "jne";
                    break;
                case expresser::JL:
                    name = "jl";
                    break;
                case expresser::JGE:
                    name = "jge";
                    break;
                case expresser::JG:
                    name = "jg";
                    break;
                case expresser::JLE:
                    name = "jle";
                    break;
                case expresser::CALL:
                    name = "call";
                    break;
                case expresser::RET:
                    name = "ret";
                    break;
                case expresser::IRET:
                    name = "iret";
                    break;
                case expresser::DRET:
                    name = "dret";
                    break;
                case expresser::ARET:
                    name = "aret";
                    break;
                case expresser::IPRINT:
                    name = "iprint";
                    break;
                case expresser::DPRINT:
                    name = "dprint";
                    break;
                case expresser::CPRINT:
                    name = "cprint";
                    break;
                case expresser::SPRINT:
                    name = "sprint";
                    break;
                case expresser::PRINTL:
                    name = "printl";
                    break;
                case expresser::ISCAN:
                    name = "iscan";
                    break;
                case expresser::DSCAN:
                    name = "dscan";
                    break;
                case expresser::CSCAN:
                    name = "cscan";
                    break;
            }
            return format_to(ctx.out(), name);
        }
    };
}
#endif //EXPRESSER_FMP_HPP
