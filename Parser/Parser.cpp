#include "Parser/Parser.h"

#include <utility>

namespace expresser {
    template<typename T>
    Constant::Constant(int index, char type, T value) {
        _index = index;
        _type = type;
        _value = value;
    }

    std::string Constant::ToCode() {
        std::string result = std::to_string(_index) + " " + _type + " ";
        switch (_type) {
            case 'D':
                result += std::get<double>(_value);
                break;
            case 'I':
                result += std::to_string(std::get<int32_t>(_value));
                break;
            case 'S':
                result += "\"" + std::get<std::string>(_value) + "\"";
                break;
            default:
                ExceptionPrint("Unknown contant type\n");
                exit(3);
        }
        return result;
    }

    std::optional<ExpresserError> Parser::Parse() {
        auto err = parseProgram();
        if (err.has_value())
            return err;
        return {};
    }

    void Parser::WriteToFile(std::ostream &_output) {
        // TODO: impl it
    }

    std::optional<Token> Parser::nextToken() {
        if (_offset == _tokens.size())
            return {};
        _current_pos = _tokens[_offset].GetEndPos();
        return _tokens[_offset++];
    }

    template<typename T>
    std::optional<ExpresserError> Parser::addGlobalConstant(const std::string &constant_name, char type, T value) {
        if (isGlobalVariable(constant_name))
            return std::make_optional<ExpresserError>(_tokens[_offset].GetStartPos(), ErrorCode::ErrDuplicateDeclaration);

        Constant constant = Constant(_global_constants.size(), type, value);
        _global_constants.insert({constant_name, constant});
        return {};
    }

    std::optional<ExpresserError> Parser::addGlobalVariable(const std::string &variable_name, std::optional<std::any> value) {
        if (isGlobalVariable(variable_name))
            return std::make_optional<ExpresserError>(_tokens[_offset].GetStartPos(), ErrorCode::ErrDuplicateDeclaration);
        // 全局堆栈上分配全局变量
        if (value->has_value()) {
            // 有初始值，用push
            auto val = std::any_cast<int32_t>(value.value());
            auto index = _start_instruments.size();
            _start_instruments.emplace_back(Instruction(index, Operation::IPUSH, 4, val));
            _global_variables.insert({variable_name, _global_sp++});
        } else {
            // 无初始值，用snew
            auto index = _start_instruments.size();
            _start_instruments.emplace_back(Instruction(index, Operation::SNEW, 4, 1));
            _global_uninitialized.insert({variable_name, _global_sp++});
        }
        return {};
    }

    std::optional<ExpresserError> Parser::addFunction(const std::string &function_name, const std::vector<FunctionParam> &params) {
        auto err = addGlobalConstant(function_name, 'S', function_name);
        if (err.has_value())
            return err.value();
        Function func(_functions.size(), _global_constants.find(function_name)->second._index, params.size(), params);
        _functions[function_name] = func;
        return {};
    }

    std::optional<ExpresserError> Parser::addFunctionInstrument(const std::string &function_name, Instruction instruction) {
        // TODO: instruction's factory
        auto err = getFunction(function_name);
        if (err.second.has_value())
            return err.second.value();
        auto func = err.first.value();
        instruction.SetIndex(func._instructions.size());
        func._instructions.push_back(instruction);
        return {};
    }

    std::optional<ExpresserError>
    Parser::addLocalConstant(const std::string &function_name, const std::string &constant_name, std::any value) {
        auto err = getFunction(function_name);
        if (err.second.has_value())
            return err.second.value();
        auto func = err.first.value();
        if (isLocalVariable(function_name, constant_name))
            return std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
        // 局部堆栈上分配局部常量
        auto val = std::any_cast<int32_t>(value);
        auto index = func._instructions.size();
        func._instructions.emplace_back(Instruction(index, Operation::IPUSH, 4, val));
        func._local_constants.insert({constant_name, _global_sp++});
        return {};
    }

    std::optional<ExpresserError>
    Parser::addLocalVariable(const std::string &function_name, const std::string &variable_name, std::optional<std::any> value) {
        auto err = getFunction(function_name);
        if (err.second.has_value())
            return err.second.value();
        auto func = err.first.value();
        if (isLocalVariable(function_name, variable_name))
            return std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
        // 局部堆栈上分配局部变量
        if (value->has_value()) {
            // 有初始值，用push
            auto val = std::any_cast<int32_t>(value.value());
            auto index = func._instructions.size();
            func._instructions.emplace_back(Instruction(index, Operation::IPUSH, 4, val));
            func._local_vars.insert({variable_name, _global_sp++});
        } else {
            // 无初始值，用snew
            auto index = func._instructions.size();
            func._instructions.emplace_back(Instruction(index, Operation::SNEW, 4, 1));
            func._local_uninitialized.insert({variable_name, _global_sp++});
        }
        return {};
    }

    std::pair<std::optional<int32_t>, std::optional<ExpresserError>>
    Parser::getIndex(const std::string &function_name, const std::string &variable_name) {
        // 先在本函数的变量区、常量区找，后在全局常量区找
        // 只返回地址，供loadc/load/store使用
        int32_t res = -1;
        if (isLocalVariable(function_name, variable_name)) {
            auto err = getFunction(function_name);
            if (err.second.has_value())
                return std::make_pair(std::optional<int32_t>(),
                                      err.second.value());
            auto func = err.first.value();
            if (func._local_constants.find(variable_name) != func._local_constants.end())
                res = func._local_constants[variable_name];
            else if (func._local_uninitialized.find(variable_name) != func._local_uninitialized.end())
                res = func._local_uninitialized[variable_name];
            else
                res = func._local_vars[variable_name];
        } else if (isGlobalVariable(variable_name)) {
            if (_global_constants.find(variable_name) != _global_constants.end())
                res = _global_constants[variable_name]._index;
            else if (_global_uninitialized.find(variable_name) != _global_uninitialized.end())
                res = _global_uninitialized[variable_name];
            else
                res = _global_variables[variable_name];
        }
        if (res != -1)
            return std::make_pair(std::make_optional<int32_t>(res),
                                  std::optional<ExpresserError>());
        return std::make_pair(std::optional<int32_t>(),
                              std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrUndeclaredIdentifier));
    }

    std::pair<std::optional<Function>, std::optional<ExpresserError>> Parser::getFunction(const std::string &function_name) {
        auto it = _functions.find(function_name);
        if (it == _functions.end())
            return std::make_pair(std::optional<Function>(),
                                  std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrUndeclaredFunction));
        return std::make_pair(it->second, std::optional<ExpresserError>());
    }

    bool Parser::isGlobalVariable(const std::string &variable_name) {
        return isGlobalInitializedVariable(variable_name) ||
               isGlobalUnInitializedVariable(variable_name) ||
               isGlobalConstant(variable_name);
    }

    bool Parser::isGlobalInitializedVariable(const std::string &variable_name) {
        return _global_variables.find(variable_name) != _global_variables.end();
    }

    bool Parser::isGlobalUnInitializedVariable(const std::string &variable_name) {
        return _global_uninitialized.find(variable_name) != _global_uninitialized.end();
    }

    bool Parser::isGlobalConstant(const std::string &variable_name) {
        return _global_constants.find(variable_name) != _global_constants.end();
    }

    bool Parser::isLocalVariable(const std::string &function_name, const std::string &variable_name) {
        return isLocalInitializedVariable(function_name, variable_name) ||
               isLocalUnInitializedVariable(function_name, variable_name) ||
               isLocalConstant(function_name, variable_name);
    }

    bool Parser::isLocalInitializedVariable(const std::string &function_name, const std::string &variable_name) {
        auto err = getFunction(function_name);
        if (!err.second.has_value())
            return false;
        auto _func = err.first.value();
        return _func._local_vars.find(variable_name) != _func._local_vars.end();
    }

    bool Parser::isLocalUnInitializedVariable(const std::string &function_name, const std::string &variable_name) {
        auto err = getFunction(function_name);
        if (!err.second.has_value())
            return false;
        auto _func = err.first.value();
        return _func._local_uninitialized.find(variable_name) != _func._local_uninitialized.end();
    }

    bool Parser::isLocalConstant(const std::string &function_name, const std::string &variable_name) {
        auto err = getFunction(function_name);
        if (!err.second.has_value())
            return false;
        auto _func = err.first.value();
        return _func._local_constants.find(variable_name) != _func._local_constants.end();
    }

    bool Parser::isInitialized(const std::string &function_name, const std::string &variable_name) {
        return isLocalInitializedVariable(function_name, variable_name) || isGlobalInitializedVariable(variable_name);
    }

    bool Parser::isUnInitialized(const std::string &function_name, const std::string &variable_name) {
        return isLocalUnInitializedVariable(function_name, variable_name) || isGlobalUnInitializedVariable(variable_name);
    }

    bool Parser::isConstant(const std::string &function_name, const std::string &variable_name) {
        return isLocalConstant(function_name, variable_name) || isGlobalConstant(variable_name);
    }

    // 语法制导翻译
    std::optional<ExpresserError> Parser::parseProgram() {
        auto err = parseGlobalDeclarations();
        if (err.has_value())
            return err;
        err = parseFunctionDefinitions();
        if (err.has_value())
            return err;
        return {};
    }

    std::optional<ExpresserError> Parser::parseGlobalDeclarations() {
        // 0个或无数个
        for (;;) {
            // TODO: impl it
            auto token = nextToken();
            if (!token.has_value())
                return {};
            if (token.value().GetType() == RESERVED && token.value().GetStringValue() == std::string("const")) {
                // TODO: const

                // TYPE
                token = nextToken();
                if (!token.has_value())
                    return std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrConstantNeedValue);
                // 只会是int/double/char中一种
                Token type = token.value();
                for (;;) {
                    // IDENTFIER
                    token = nextToken();
                    if (!token.has_value() || token.value().GetType() != IDENTIFIER)
                        return std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrNeedIdentifier);
                    Token identifier = token.value();

                    // =
                    token = nextToken();
                    if (!token.has_value() || token.value().GetType() != ASSIGN)
                        return std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrInvalidAssignment);

                    // <expression>
                    // TODO: 将结果存入常量表中
                    auto err = parseExpression(stringTypeToTokenType(type.GetStringValue()).value());
                    if (err.has_value())
                        return err.value();

                    // , or ;
                    token = nextToken();
                    if (token.has_value()) {
                        if (token.value().GetType() == SEMICOLON)
                            break;
                        if (token.value().GetType() == COLON)
                            continue;
                    }
                    // 不是分号逗号、或者没值
                    return std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrInvalidAssignment);
                }
            } else if (token.value().GetType() == RESERVED && token.value().GetStringValue() == std::string("const")) {
                // TODO: var or func
            }
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseFunctionDefinitions() {
        // 1个或无数个
        for (;;) {
            // TODO: impl it
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseFunctionDefinition() {
        //<function-definition> ::
        //    <type-specifier><identifier><parameter-clause><compound-statement>
        return {};
    }

    std::optional<ExpresserError> Parser::parseParameterDeclarations() {
        //<parameter-clause> ::=
        //    '(' [<parameter-declaration-list>] ')'
        //<parameter-declaration-list> ::=
        //    <parameter-declaration>{','<parameter-declaration>}
        return {};
    }

    std::optional<ExpresserError> Parser::parseCompoundStatement() {
        // 拓展C0,作用域
        //<compound-statement> ::=
        //    '{' {<variable-declaration>} <statement-seq> '}'
        //<statement-seq> ::=
        //	{<statement>}
        return {};
    }

    std::optional<ExpresserError> Parser::parseLocalVariableDeclarations() {
        return {};
    }

    std::optional<ExpresserError> Parser::parseStatements() {
        //<statement-seq> ::=
        //	{<statement>}
        //<statement> ::=
        //     <compound-statement>   --   拓展C0,作用域
        //    |<condition-statement>
        //    |<loop-statement>
        //    |<jump-statement>
        //    |<print-statement>
        //    |<scan-statement>
        //    |<assignment-expression>';'
        //    |<function-call>';'
        //    |';'
        // 0或多个
        for (;;) {

        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseConditionStatement() {
        //<condition> ::=
        //     <expression>[<relational-operator><expression>]
        //<condition-statement> ::=
        //     'if' '(' <condition> ')' <statement> ['else' <statement>]
        return {};
    }

    std::optional<ExpresserError> Parser::parseLoopStatement() {
        //<loop-statement> ::=
        //    'while' '(' <condition> ')' <statement>
        return {};
    }

    std::optional<ExpresserError> Parser::parseJumpStatement() {
        //<jump-statement> ::=
        //    <return-statement>
        //<return-statement> ::=
        //    'return' [<expression>] ';'
        return {};
    }

    std::optional<ExpresserError> Parser::parsePrintStatement() {
        //<print-statement> ::=
        //    'print' '(' [<printable-list>] ')' ';'
        //<printable-list>  ::=
        //    <printable> {',' <printable>}
        //<printable> ::=
        //    <expression> | <string-literal> | <char-literal>   --   拓展C0，字面量打印
        return {};
    }

    std::optional<ExpresserError> Parser::parseScanStatement() {
        //<scan-statement> ::=
        //    'scan' '(' <identifier> ')' ';'
        return {};
    }

    std::optional<ExpresserError> Parser::parseAssignmentExpression() {
        //<assignment-expression> ::=
        //    <identifier><assignment-operator><expression>
        return {};
    }

    std::optional<ExpresserError> Parser::parseExpression(TokenType type) {
        // TODO: impl it
        //<expression> ::=
        //    <additive-expression>
        //<additive-expression> ::=
        //     <multiplicative-expression>{<additive-operator><multiplicative-expression>}
        return {};
    }

    std::optional<ExpresserError> Parser::parseMultiplicativeExpression(TokenType type) {
        // TODO: impl it
        // 拓展C0，类型转换
        //<multiplicative-expression> ::=
        //     <cast-expression>{<multiplicative-operator><cast-expression>}
        return {};
    }

    std::optional<ExpresserError> Parser::parseUnaryExpression(TokenType type) {
        // TODO: impl it
        //<unary-expression> ::=
        //    [<unary-operator>]<primary-expression>
        return {};
    }

    std::optional<ExpresserError> Parser::parsePrimaryExpression(TokenType type) {
        // TODO: impl it
        //<primary-expression> ::=
        //     '('<expression>')'
        //    |<identifier>
        //    |<integer-literal>
        //    |<function-call>
        //    |<char-literal>   --   拓展C0，char
        return {};
    }

    std::optional<ExpresserError> Parser::parseFunctionCall() {
        //<function-call> ::=
        //    <identifier> '(' [<expression-list>] ')'
        //<expression-list> ::=
        //    <expression>{','<expression>}
        return {};
    }

    std::optional<ExpresserError> Parser::parseCastExpression() {
        // 拓展C0，类型转换
        //<cast-expression> ::=
        //    {'('<type-specifier>')'}<unary-expression>
        return {};
    }

    bool Parser::isVariableType(const Token &token) {
        auto it = _variable_type_map.find(token.GetStringValue());
        return it != _variable_type_map.end();
    }

    std::optional<TokenType> Parser::stringTypeToTokenType(const std::string &type_name) {
        auto it = _variable_type_map.find(type_name);
        if (it == _variable_type_map.end())
            return it->second;
        return {};
    }
}