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
        ErrConstantNeedValue,
        ErrDoubleOverflow,
        ErrDuplicateDeclaration,
        ErrEOF,
        ErrIncompleteExpression,
        ErrIncompleteFunction,
        ErrInvalidAssignment,
        ErrInvalidCharacter,
        ErrInvalidDouble,
        ErrInvalidFunctionDeclaration,
        ErrInvalidIdentifier,
        ErrInvalidInput,
        ErrInvalidInteger,
        ErrInvalidNotEqual,
        ErrInvalidPrint,
        ErrInvalidScan,
        ErrInvalidStringLiteral,
        ErrInvalidVariableDeclaration,
        ErrIntegerOverflow,
        ErrNeedIdentifier,
        ErrNoSemicolon,
        ErrNotInitialized,
        ErrNotDeclared,
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
                _pos(std::move(position)), _code(code) {}
        ExpresserError(uint32_t line, uint32_t line_pos, ErrorCode code) :
                _pos({line, line_pos}), _code(code) {}
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
