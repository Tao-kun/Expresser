#ifndef EXPRESSER_ERROR_H
#define EXPRESSER_ERROR_H

#include <iostream>
#include <string>
#include <utility>

#include "Types.h"

namespace expresser {
    inline void ExceptionPrint(const std::string &cond) {
        std::cout << "Exception: " << cond << std::endl;
        std::abort();
    }

    enum ErrorCode : char {
        ErrAssignToConstant,
        ErrCastToVoid,
        ErrConstantNeedValue,
        ErrDoubleOverflow,
        ErrDuplicateDeclaration,
        ErrEOF,
        ErrIncompleteExpression,
        ErrIncompleteFunction,
        ErrInvalidAssignment,
        ErrInvalidCharacter,
        ErrInvalidCharacterAssignment,
        ErrInvalidDouble,
        ErrInvalidExpression,
        ErrInvalidFunctionDeclaration,
        ErrInvalidFunctionReturnType,
        ErrInvalidIdentifier,
        ErrInvalidInput,
        ErrInvalidInteger,
        ErrInvalidJump,
        ErrInvalidLoop,
        ErrInvalidNotEqual,
        ErrInvalidParameter,
        ErrInvalidPrint,
        ErrInvalidScan,
        ErrInvalidStatement,
        ErrInvalidStringLiteral,
        ErrInvalidVariableDeclaration,
        ErrInvalidVariableType,
        ErrIntegerOverflow,
        ErrMissingBrace,
        ErrMissingBracket,
        ErrNeedAssignSymbol,
        ErrNeedIdentifier,
        ErrNeedRelationalOperator,
        ErrNeedVariableType,
        ErrNoSemicolon,
        ErrNotInitialized,
        ErrNotDeclared,
        ErrReturnInVoidFunction,
        ErrStreamError,
        ErrUndeclaredFunction,
        ErrUndeclaredIdentifier,
    };

    class ExpresserError final {
    private:
        ErrorCode _code;
        position_t _pos;
    public:
        ExpresserError(position_t position, ErrorCode code) :
                _code(code), _pos(std::move(position)) {}

        ExpresserError(uint32_t line, uint32_t line_pos, ErrorCode code) :
                _code(code), _pos({line, line_pos}) {}

        ExpresserError &operator=(ExpresserError ce) {
            std::swap(*this, ce);
            return *this;
        }

        bool operator==(const ExpresserError &rhs) const { return _pos == rhs._pos && _code == rhs._code; }

        position_t GetPos() const { return _pos; }

        ErrorCode GetCode() const { return _code; }
    };
}
#endif //EXPRESSER_ERROR_H
