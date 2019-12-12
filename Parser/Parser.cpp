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

    std::optional<Token> Parser::seekToken(int32_t offset) {
        if (_offset + offset >= _tokens.size())
            return {};
        return _tokens[_offset + offset];
    }

    void Parser::rollback() {
        if (_offset <= 0) {
            std::cerr << "Cannot rollback" << std::endl;
            exit(2);
        }
        _offset--;
    }

    template<typename T>
    std::optional<ExpresserError> Parser::addGlobalConstant(const std::string &constant_name, char type, T value) {
        if (isGlobalVariable(constant_name))
            return errorFactory(ErrorCode::ErrDuplicateDeclaration);

        Constant constant = Constant(_global_constants.size(), type, value);
        _global_constants.insert({constant_name, constant});
        return {};
    }

    std::optional<ExpresserError>
    Parser::addGlobalVariable(const std::string &variable_name, TokenType type, std::optional<std::any> value) {
        if (isGlobalVariable(variable_name))
            return errorFactory(ErrorCode::ErrDuplicateDeclaration);
        // 全局堆栈上分配全局变量
        if (value->has_value()) {
            // 有初始值，用push
            auto val = std::any_cast<int32_t>(value.value());
            auto index = _start_instruments.size();
            _start_instruments.emplace_back(Instruction(index, Operation::IPUSH, 4, val));
            _global_variables.insert({variable_name, {_global_sp++, type}});
        } else {
            // 无初始值，用snew
            auto index = _start_instruments.size();
            _start_instruments.emplace_back(Instruction(index, Operation::SNEW, 4, 1));
            _global_uninitialized.insert({variable_name, {_global_sp++, type}});
        }
        return {};
    }

    std::optional<ExpresserError>
    Parser::addFunction(const std::string &function_name, const TokenType &return_type, const std::vector<FunctionParam> &params) {
        auto err = addGlobalConstant(function_name, 'S', function_name);
        if (err.has_value())
            return err.value();
        Function func(_functions.size(), _global_constants.find(function_name)->second._index, params.size(), return_type, params);
        _functions[function_name] = func;
        // 把参数信息放入函数变量、常量表中
        for (int32_t i = 0; i < params.size(); i++) {
            auto p = params[i];
            if (p._is_const)
                func._local_constants.insert({p._value, {i, p._type}});
            else
                func._local_vars.insert({p._value, {i, p._type}});
        }
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
    Parser::addLocalConstant(const std::string &function_name, TokenType type, const std::string &constant_name, std::any value) {
        auto err = getFunction(function_name);
        if (err.second.has_value())
            return err.second.value();
        auto func = err.first.value();
        if (isLocalVariable(function_name, constant_name))
            return errorFactory(ErrorCode::ErrDuplicateDeclaration);
        // 局部堆栈上分配局部常量
        auto val = std::any_cast<int32_t>(value);
        auto index = func._instructions.size();
        func._instructions.emplace_back(Instruction(index, Operation::IPUSH, 4, val));
        func._local_constants.insert({constant_name, {_global_sp++, type}});
        return {};
    }

    std::optional<ExpresserError>
    Parser::addLocalVariable(const std::string &function_name, const std::string &variable_name,
                             TokenType type, std::optional<std::any> value) {
        auto err = getFunction(function_name);
        if (err.second.has_value())
            return err.second.value();
        auto func = err.first.value();
        if (isLocalVariable(function_name, variable_name))
            return errorFactory(ErrorCode::ErrDuplicateDeclaration);
        // 局部堆栈上分配局部变量
        if (value->has_value()) {
            // 有初始值，用push
            auto val = std::any_cast<int32_t>(value.value());
            auto index = func._instructions.size();
            func._instructions.emplace_back(Instruction(index, Operation::IPUSH, 4, val));
            func._local_vars.insert({variable_name, {_global_sp++, type}});
        } else {
            // 无初始值，用snew
            auto index = func._instructions.size();
            func._instructions.emplace_back(Instruction(index, Operation::SNEW, 4, 1));
            func._local_uninitialized.insert({variable_name, {_global_sp++, type}});
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
                res = func._local_constants[variable_name]._index;
            else if (func._local_uninitialized.find(variable_name) != func._local_uninitialized.end())
                res = func._local_uninitialized[variable_name]._index;
            else
                res = func._local_vars[variable_name]._index;
        } else if (isGlobalVariable(variable_name)) {
            if (_global_constants.find(variable_name) != _global_constants.end())
                res = _global_constants[variable_name]._index;
            else if (_global_uninitialized.find(variable_name) != _global_uninitialized.end())
                res = _global_uninitialized[variable_name]._index;
            else
                res = _global_variables[variable_name]._index;
        }
        if (res != -1)
            return std::make_pair(std::make_optional<int32_t>(res),
                                  std::optional<ExpresserError>());
        return std::make_pair(std::optional<int32_t>(),
                              errorFactory(ErrorCode::ErrUndeclaredIdentifier));
    }

    std::pair<std::optional<Function>, std::optional<ExpresserError>> Parser::getFunction(const std::string &function_name) {
        auto it = _functions.find(function_name);
        if (it == _functions.end())
            return std::make_pair(std::optional<Function>(),
                                  errorFactory(ErrorCode::ErrUndeclaredFunction));
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

    std::optional<ExpresserError> Parser::errorFactory(ErrorCode code) {
        return std::make_optional<ExpresserError>(_current_pos, code);
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
        //<variable-declaration> ::=
        //    [<const-qualifier>]<type-specifier><init-declarator-list>';'
        //<init-declarator-list> ::=
        //    <init-declarator>{','<init-declarator>}
        //<init-declarator> ::=
        //    <identifier>[<initializer>]
        //<initializer> ::=
        //    '='<expression>
        // 0个或无数个
        for (;;) {
            // TODO: impl it
            auto token = nextToken();
            if (!token.has_value())
                return {};
            if (token->GetType() == RESERVED && token->GetStringValue() == std::string("const")) {
                // TYPE
                token = nextToken();
                if (!token.has_value())
                    return errorFactory(ErrorCode::ErrConstantNeedValue);
                // 只会是int/char中一种
                Token type = token.value();
                TokenType const_type = stringTypeToTokenType(type.GetStringValue()).value();
                for (;;) {
                    // IDENTFIER
                    token = nextToken();
                    if (!token.has_value() || token->GetType() != IDENTIFIER)
                        return errorFactory(ErrorCode::ErrNeedIdentifier);
                    Token identifier = token.value();

                    // =
                    token = nextToken();
                    if (!token.has_value() || token->GetType() != ASSIGN)
                        return errorFactory(ErrorCode::ErrInvalidAssignment);

                    // <expression>
                    // TODO: 将结果存入常量表中
                    auto err = parseExpression(const_type);
                    if (err.has_value())
                        return err.value();

                    // , or ;
                    token = nextToken();
                    if (token.has_value()) {
                        if (token->GetType() == SEMICOLON)
                            break;
                        if (token->GetType() == COMMA)
                            continue;
                    }
                    // 不是分号逗号、或者没值
                    return errorFactory(ErrorCode::ErrInvalidAssignment);
                }
            } else if (token->GetType() == RESERVED && isVariableType(token.value())) {
                // 超前扫描，确认是否是函数的声明
                auto seek = seekToken(2);
                if (seek.has_value() && seek->GetType() == LEFTBRACKET) {
                    // 回滚一个token
                    rollback();
                    return {};
                }
                auto type = token.value();
                TokenType var_type = stringTypeToTokenType(type.GetStringValue()).value();
                for (;;) {
                    // IDENTFIER
                    token = nextToken();
                    if (!token.has_value() || token->GetType() != IDENTIFIER)
                        return errorFactory(ErrorCode::ErrNeedIdentifier);
                    auto identifier = token.value().GetStringValue();
                    // snew分配空间，因为不支持double所以slot为4
                    addGlobalVariable(identifier, var_type, {});

                    // =
                    token = nextToken();
                    if (!token.has_value())
                        return errorFactory(ErrorCode::ErrEOF);
                    else if (token->GetType() == COMMA)
                        continue;
                    else if (token->GetType() == SEMICOLON)
                        break;
                    else if (token->GetType() == ASSIGN) {
                        // 加载预分配空间在栈中地址
                        auto index = _start_instruments.size();
                        auto it = _global_uninitialized.find(identifier);
                        _start_instruments.emplace_back(Instruction(index, Operation::LOADA, 2, 0, 4, it->second._index));
                        // 解析<expression>
                        auto err = parseExpression(var_type);
                        if (err.has_value())
                            return err.value();
                        // 此时全局栈的栈顶就是结果，次栈顶是目标地址
                        // istore
                        index = _start_instruments.size();
                        _start_instruments.emplace_back(Instruction(index, Operation::ISTORE));
                        if (isGlobalUnInitializedVariable(identifier)) {
                            it = _global_uninitialized.find(identifier);
                            // 移入_global_variables
                            _global_variables.insert({it->first, it->second});
                            // 移出_global_uninitialized
                            _global_uninitialized.erase(identifier);
                        }

                        // , or ;
                        token = nextToken();
                        if (token.has_value()) {
                            if (token->GetType() == SEMICOLON)
                                break;
                            if (token->GetType() == COMMA)
                                continue;
                        }
                        // 不是分号逗号、或者没值
                        return errorFactory(ErrorCode::ErrInvalidAssignment);
                    } else
                        return errorFactory(ErrorCode::ErrInvalidAssignment);
                }
            } else
                return errorFactory(ErrorCode::ErrInvalidInput);
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseFunctionDefinitions() {
        // 1个或无数个
        for (;;) {
            auto err = parseFunctionDefinition();
            if (err.has_value())
                return err.value();
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseFunctionDefinition() {
        //<function-definition> ::
        //    <type-specifier><identifier><parameter-clause><compound-statement>

        // <type-specifier>
        auto token = nextToken();
        if (!token.has_value() || token->GetType() != TokenType::RESERVED || !isVariableType(token.value()))
            return errorFactory(ErrorCode::ErrInvalidFunctionDeclaration);
        auto type = stringTypeToTokenType(token->GetStringValue());
        if (!type.has_value())
            return errorFactory(ErrorCode::ErrInvalidFunctionReturnType);
        auto return_type = type.value();

        // <identifier>
        token = nextToken();
        if (!token.has_value() || token->GetType() != TokenType::IDENTIFIER)
            return errorFactory(ErrorCode::ErrNeedIdentifier);
        auto function_name = token->GetStringValue();

        // <parameter-clause>
        std::vector<FunctionParam> params;
        auto err = parseParameterDeclarations(params);
        if (err.has_value())
            return err.value();
        err = addFunction(function_name, return_type, params);
        if (err.has_value())
            return err.value();

        // <compound-statement>
        err = parseCompoundStatement();
        if (err.has_value())
            return err.value();

        return {};
    }

    std::optional<ExpresserError> Parser::parseParameterDeclarations(std::vector<FunctionParam> &params) {
        //<parameter-clause> ::=
        //    '(' [<parameter-declaration-list>] ')'
        //<parameter-declaration-list> ::=
        //    <parameter-declaration>{','<parameter-declaration>}
        //<parameter-declaration> ::=
        //    [<const-qualifier>]<type-specifier><identifier>
        auto token = nextToken();
        // 左括号
        if (!token.has_value() || token->GetType() != LEFTBRACKET)
            return errorFactory(ErrorCode::ErrMissingBracket);
        token = nextToken();
        for (;; token = nextToken()) {
            if (!token.has_value())
                return errorFactory(ErrorCode::ErrEOF);
            else if (token->GetType() == RIGHTBRACKET)
                // 右括号，退出
                break;
            else if (token->GetType() == RESERVED && (isVariableType(token.value()) || token->GetStringValue() == "const")) {
                bool is_const = false;
                if (token->GetStringValue() == "const") {
                    token = nextToken();
                    if (!token.has_value())
                        return errorFactory(ErrorCode::ErrEOF);
                    if (token->GetType() != RESERVED || !isVariableType(token.value()))
                        return errorFactory(ErrorCode::ErrInvalidVariableType);
                    is_const = true;
                }
                // <类型><标识符>
                auto param_type = stringTypeToTokenType(token->GetStringValue()).value();
                token = nextToken();
                if (!token.has_value() || token->GetType() != IDENTIFIER)
                    return errorFactory(ErrorCode::ErrNeedIdentifier);
                params.emplace_back(FunctionParam(param_type, token->GetStringValue(), is_const));
                // 逗号继续
                // 右括号退出
                // 其他报错
                token = nextToken();
                if (!token.has_value())
                    return errorFactory(ErrorCode::ErrEOF);
                else if (token->GetType() == RIGHTBRACKET)
                    break;
                else if (token->GetType() == COMMA)
                    continue;
                else
                    return errorFactory(ErrorCode::ErrInvalidParameter);
            } else
                return errorFactory(ErrorCode::ErrInvalidParameter);
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseCompoundStatement() {
        //<compound-statement> ::=
        //    '{' {<variable-declaration>} <statement-seq> '}'
        //<statement-seq> ::=
        //	{<statement>}
        auto token = nextToken();
        if (!token.has_value() || token->GetType() != LEFTBRACE)
            return errorFactory(ErrorCode::ErrMissingBrace);
        auto err = parseLocalVariableDeclarations();
        if (err.has_value())
            return err;
        err = parseStatements();
        if (err.has_value())
            return err;
        token = nextToken();
        if (!token.has_value() || token->GetType() != LEFTBRACE)
            return errorFactory(ErrorCode::ErrMissingBrace);
        return {};
    }

    std::optional<ExpresserError> Parser::parseLocalVariableDeclarations() {
        // TODO:impl it
        //<variable-declaration> ::=
        //    [<const-qualifier>]<type-specifier><init-declarator-list>';'
        //<init-declarator-list> ::=
        //    <init-declarator>{','<init-declarator>}
        //<init-declarator> ::=
        //    <identifier>[<initializer>]
        //<initializer> ::=
        //    '='<expression>
        // seekToken预读
        return {};
    }

    std::optional<ExpresserError> Parser::parseStatements() {
        //<statement-seq> ::=
        //	{<statement>}
        //<statement> ::=
        //     '{' <statement-seq> '}'
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
        //   |'do' <statement> 'while' '(' <condition> ')' ';'   --   拓展C0，`do...while`
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
        // 传入的token._type==RESERVED
        return isFunctionReturnType(token) && token.GetStringValue() != "void";
    }

    bool Parser::isFunctionReturnType(const Token &token) {
        auto it = _function_return_type_map.find(token.GetStringValue());
        return it != _function_return_type_map.end();
    }

    std::optional<TokenType> Parser::stringTypeToTokenType(const std::string &type_name) {
        auto it = _function_return_type_map.find(type_name);
        if (it == _function_return_type_map.end())
            return it->second;
        return {};
    }
}