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
        // 一般只用来字符串
        int32_t index = _global_constants.size();
        _global_constants.emplace_back(Constant(index, type, value));
        _global_constants_index.insert({constant_name, index});
        return {};
    }

    std::optional<ExpresserError> Parser::addGlobalConstant(const std::string &variable_name, TokenType type) {
        if (isGlobalVariable(variable_name))
            return errorFactory(ErrorCode::ErrDuplicateDeclaration);
        // 只负责加入_global_stack_constants
        auto index = _start_instruments.size();
        _start_instruments.emplace_back(Instruction(index, Operation::SNEW, 4, 1));
        _global_stack_constants.insert({variable_name, {_global_sp++, type}});
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
        Function func(_functions.size(), _global_constants_index.find(function_name)->second, params.size(), return_type, params);
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
        func._local_constants.insert({constant_name, {func._local_sp++, type}});
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
            func._local_vars.insert({variable_name, {func._local_sp++, type}});
        } else {
            // 无初始值，用snew
            auto index = func._instructions.size();
            func._instructions.emplace_back(Instruction(index, Operation::SNEW, 4, 1));
            func._local_uninitialized.insert({variable_name, {func._local_sp++, type}});
        }
        return {};
    }

    std::pair<std::optional<int32_t>, std::optional<ExpresserError>> Parser::getIndex(const std::string &variable_name) {
        int32_t res = -1;
        if (isGlobalVariable(variable_name)) {
            if (_global_constants_index.find(variable_name) != _global_constants_index.end())
                res = _global_constants_index[variable_name];
            else if (_global_stack_constants.find(variable_name) != _global_stack_constants.end())
                res = _global_stack_constants[variable_name]._index;
            else if (_global_uninitialized.find(variable_name) != _global_uninitialized.end())
                res = _global_uninitialized[variable_name]._index;
            else
                res = _global_variables[variable_name]._index;
        }
        if (res != -1)
            return std::make_pair(std::make_optional<int32_t>(res), std::optional<ExpresserError>());
        return std::make_pair(std::optional<int32_t>(), errorFactory(ErrorCode::ErrUndeclaredIdentifier));
    }

    std::pair<std::optional<int32_t>, std::optional<ExpresserError>>
    Parser::getIndex(const std::string &function_name, const std::string &variable_name) {
        // 先在本函数的变量区、常量区找，后在全局常量区找
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
            return getIndex(variable_name);
        }
        if (res != -1)
            return std::make_pair(std::make_optional<int32_t>(res), std::optional<ExpresserError>());
        return std::make_pair(std::optional<int32_t>(), errorFactory(ErrorCode::ErrUndeclaredIdentifier));
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
        return _global_constants_index.find(variable_name) != _global_constants_index.end() ||
               _global_stack_constants.find(variable_name) != _global_stack_constants.end();
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
        return isLocalConstant(function_name, variable_name) ||
               isGlobalConstant(variable_name);
    }

    std::optional<TokenType> Parser::getVariableType(const std::string &variable_name) {
        if (isGlobalConstant(variable_name))
            return _global_stack_constants[variable_name]._type;
        if (isGlobalVariable(variable_name))
            return _global_variables[variable_name]._type;
        if (isGlobalUnInitializedVariable(variable_name))
            return _global_uninitialized[variable_name]._type;
        return {};
    }

    std::optional<TokenType> Parser::getVariableType(const std::string &function_name, const std::string &variable_name) {
        auto err = getFunction(function_name);
        if (!err.second.has_value())
            return {};
        auto _func = err.first.value();
        if (isLocalConstant(function_name, variable_name))
            return _func._local_constants[variable_name]._type;
        if (isLocalVariable(function_name, variable_name))
            return _func._local_vars[variable_name]._type;
        if (isLocalUnInitializedVariable(function_name, variable_name))
            return _func._local_uninitialized[variable_name]._type;
        return {};
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
            auto token = nextToken();
            if (!token.has_value())
                return {};
            if (token->GetType() == RESERVED && token->GetStringValue() == std::string("const")) {
                // TYPE
                token = nextToken();
                if (!token.has_value())
                    return errorFactory(ErrorCode::ErrConstantNeedValue);
                // 只会是int/char中一种
                TokenType const_type = stringTypeToTokenType(token->GetStringValue()).value();
                for (;;) {
                    // IDENTFIER
                    token = nextToken();
                    if (!token.has_value() || token->GetType() != IDENTIFIER)
                        return errorFactory(ErrorCode::ErrNeedIdentifier);
                    std::string identifier = token->GetStringValue();
                    if (isGlobalVariable(identifier))
                        return errorFactory(ErrorCode::ErrDuplicateDeclaration);
                    addGlobalConstant(identifier, const_type);

                    // =
                    token = nextToken();
                    if (!token.has_value() || token->GetType() != ASSIGN)
                        return errorFactory(ErrorCode::ErrInvalidAssignment);

                    // LOADA取地址
                    auto const_index = getIndex(identifier).first.value();
                    auto index = _start_instruments.size();
                    _start_instruments.emplace_back(Instruction(index, Operation::LOADA, 2, 0, 4, const_index));

                    // 解析<expression>
                    auto err = parseExpression(const_type, {});
                    if (err.has_value())
                        return err.value();

                    // ISTORE存回
                    // 此时栈顶为<expression>结果，次栈顶为LOADA取出的地址
                    _start_instruments.emplace_back(Instruction(index + 1, Operation::ISTORE));

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
                TokenType var_type = stringTypeToTokenType(token->GetStringValue()).value();
                for (;;) {
                    // IDENTFIER
                    token = nextToken();
                    if (!token.has_value() || token->GetType() != IDENTIFIER)
                        return errorFactory(ErrorCode::ErrNeedIdentifier);
                    auto identifier = token->GetStringValue();
                    if (isGlobalVariable(identifier))
                        return errorFactory(ErrorCode::ErrDuplicateDeclaration);
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
                        auto var_index = _global_sp - 1;
                        _start_instruments.emplace_back(Instruction(index, Operation::LOADA, 2, 0, 4, var_index));

                        // 解析<expression>
                        auto err = parseExpression(var_type, {});
                        if (err.has_value())
                            return err.value();

                        // 此时全局栈的栈顶就是结果，次栈顶是目标地址
                        // istore
                        _start_instruments.emplace_back(Instruction(index + 1, Operation::ISTORE));
                        // 移出未初始化表
                        auto it = _global_uninitialized.find(identifier);
                        // 移入_global_variables
                        _global_variables.insert({it->first, it->second});
                        // 移出_global_uninitialized
                        _global_uninitialized.erase(identifier);

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
                // 可能是函数声明，先不报错
                return {};
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
        auto function = _functions[function_name];
        err = parseCompoundStatement(function);
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

    std::optional<ExpresserError> Parser::parseCompoundStatement(Function &function) {
        //<compound-statement> ::=
        //    '{' {<variable-declaration>} <statement-seq> '}'
        //<statement-seq> ::=
        //	{<statement>}
        auto token = nextToken();
        if (!token.has_value() || token->GetType() != LEFTBRACE)
            return errorFactory(ErrorCode::ErrMissingBrace);
        auto err = parseLocalVariableDeclarations(function);
        if (err.has_value())
            return err;
        err = parseStatements(function);
        if (err.has_value())
            return err;
        token = nextToken();
        if (!token.has_value() || token->GetType() != RIGHTBRACE)
            return errorFactory(ErrorCode::ErrMissingBrace);
        return {};
    }

    std::optional<ExpresserError> Parser::parseLocalVariableDeclarations(Function &function) {
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
        auto token = nextToken();
        if (!token.has_value())
            return {};

        auto function_name = std::get<std::string>(_global_constants[function._name_index]._value);
        if (token->GetType() == RESERVED && token->GetStringValue() == "const") {
            // const
            token = nextToken();
            if (!token.has_value())
                return errorFactory(ErrorCode::ErrConstantNeedValue);
            TokenType const_type = stringTypeToTokenType(token->GetStringValue()).value();
            for (;;) {
                token = nextToken();
                if (!token.has_value() || token->GetType() != IDENTIFIER)
                    return errorFactory(ErrorCode::ErrNeedIdentifier);
                std::string identifier = token->GetStringValue();
                if (isLocalVariable(function_name, identifier))
                    return errorFactory(ErrorCode::ErrDuplicateDeclaration);
                addLocalConstant(function_name, const_type, identifier, {});

                token = nextToken();
                if (!token.has_value() || token->GetType() != ASSIGN)
                    return errorFactory(ErrorCode::ErrNeedAssignSymbol);

                auto const_index = getIndex(function_name, identifier).first.value();
                auto index = function._instructions.size();
                function._instructions.emplace_back(Instruction(index, Operation::LOADA, 2, 0, 4, const_index));

                auto err = parseExpression(const_type, function);
                if (err.has_value())
                    return err;

                function._instructions.emplace_back(Instruction(index + 1, Operation::ISTORE));

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
        } else if (token->GetType() == RESERVED || isVariableType(token.value())) {
            // var or func
            // 预读判断函数
            auto seek = seekToken(2);
            if (seek.has_value() && seek->GetType() == LEFTBRACKET) {
                // 回滚一个token
                rollback();
                return {};
            }
            TokenType var_type = stringTypeToTokenType(token->GetStringValue()).value();
            for (;;) {
                if (!token.has_value() || token->GetType() != IDENTIFIER)
                    return errorFactory(ErrorCode::ErrNeedIdentifier);
                auto identifier = token->GetStringValue();
                if (isLocalVariable(function_name, identifier))
                    return errorFactory(ErrorCode::ErrDuplicateDeclaration);
                addLocalConstant(function_name, var_type, identifier, {});

                token = nextToken();
                if (!token.has_value())
                    return errorFactory(ErrorCode::ErrEOF);
                else if (token->GetType() == COMMA)
                    continue;
                else if (token->GetType() == SEMICOLON)
                    break;
                else if (token->GetType() == ASSIGN) {
                    auto index = function._instructions.size();
                    auto var_index = function._local_sp - 1;
                    function._instructions.emplace_back(Instruction(index, Operation::LOADA, 2, 0, 4, var_index));

                    auto err = parseExpression(var_type, function);
                    if (err.has_value())
                        return err.value();

                    function._instructions.emplace_back(Instruction(index + 1, Operation::ISTORE));
                    auto it = function._local_uninitialized.find(identifier);
                    function._local_vars.insert({it->first, it->second});
                    function._local_uninitialized.erase(identifier);

                    token = nextToken();
                    if (token.has_value()) {
                        if (token->GetType() == SEMICOLON)
                            break;
                        if (token->GetType() == COMMA)
                            continue;
                    }
                    return errorFactory(ErrorCode::ErrInvalidAssignment);
                } else
                    return errorFactory(ErrorCode::ErrInvalidAssignment);
            }
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseStatements(Function &function) {
        //<statement-seq> ::=
        //	{<statement>}
        //<statement> ::=
        //     '{' <statement-seq> '}'   --   不支持多级作用域
        //    |<condition-statement>
        //    |<loop-statement>
        //    |<jump-statement>
        //    |<print-statement>
        //    |<scan-statement>
        //    |<assignment-expression>';'
        //    |<function-call>';'
        //    |';'
        // 0或多个
        std::optional<Token> token;
        for (;;) {
            token = nextToken();
            if (!token.has_value())
                return errorFactory(ErrorCode::ErrEOF);
            if (token->GetType() == RIGHTBRACE) {
                // 右括号，函数结束
                // 如果是void函数，添加ret
                rollback();
                return {};
            }
            if (token->GetType() == RESERVED && token->GetStringValue() == "if") {
                rollback();
                auto err = parseConditionStatement(function);
                if (err.has_value())
                    return err.value();
            } else if (token->GetType() == RESERVED && (token->GetStringValue() == "do" || token->GetStringValue() == "while")) {
                rollback();
                auto err = parseLoopStatement(function);
                if (err.has_value())
                    return err.value();
            } else if (token->GetType() == RESERVED && token->GetStringValue() == "return") {
                rollback();
                auto err = parseJumpStatement(function);
                if (err.has_value())
                    return err.value();
            } else if (token->GetType() == RESERVED && token->GetStringValue() == "print") {
                rollback();
                auto err = parsePrintStatement(function);
                if (err.has_value())
                    return err.value();
            } else if (token->GetType() == RESERVED && token->GetStringValue() == "scan") {
                rollback();
                auto err = parseScanStatement(function);
                if (err.has_value())
                    return err.value();
            } else if (token->GetType() == IDENTIFIER) {
                // TODO: function call or assign
                auto seek = seekToken(1);
                std::optional<ExpresserError> err;
                if (!seek.has_value() ||
                    (seek->GetType() != LEFTBRACKET && seek->GetType() != IDENTIFIER))
                    return errorFactory(ErrorCode::ErrEOF);
                rollback();
                if (seek->GetType() == LEFTBRACKET)
                    err = parseFunctionCall(function);
                else
                    err = parseAssignmentExpression(function);
                if (err.has_value())
                    return err.value();
            } else if (token->GetType() == SEMICOLON) {
                continue;
            } else {
                return errorFactory(ErrorCode::ErrInvalidStatement);
            }
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseConditionStatement(Function &function) {
        //<condition> ::=
        //     <expression>[<relational-operator><expression>]
        //<condition-statement> ::=
        //     'if' '(' <condition> ')' <statement> ['else' <statement>]
        return {};
    }

    std::optional<ExpresserError> Parser::parseLoopStatement(Function &function) {
        //<loop-statement> ::=
        //    'while' '(' <condition> ')' <statement>
        //   |'do' <statement> 'while' '(' <condition> ')' ';'   --   拓展C0，`do...while`
        return {};
    }

    std::optional<ExpresserError> Parser::parseJumpStatement(Function &function) {
        //<jump-statement> ::=
        //    <return-statement>
        //<return-statement> ::=
        //    'return' [<expression>] ';'
        // 先跳过'return'
        auto return_type = function._return_type;
        auto token = nextToken();
        token = nextToken();
        if (!token.has_value())
            return errorFactory(ErrorCode::ErrEOF);
        if (return_type == VOID) {
            if (token->GetType() != SEMICOLON)
                return errorFactory(ErrorCode::ErrReturnInVoidFunction);
            auto index = function._instructions.size();
            function._instructions.emplace_back(Instruction(index, Operation::RET));
        }

        rollback();
        auto err = parseExpression(return_type, function);
        if (err.has_value())
            return err.value();
        auto index = function._instructions.size();
        function._instructions.emplace_back(Instruction(index, Operation::IRET));
        return {};
    }

    std::optional<ExpresserError> Parser::parsePrintStatement(Function &function) {
        //<print-statement> ::=
        //    'print' '(' [<printable-list>] ')' ';'
        //<printable-list>  ::=
        //    <printable> {',' <printable>}
        //<printable> ::=
        //    <expression> | <string-literal> | <char-literal>   --   拓展C0，字面量打印
        return {};
    }

    std::optional<ExpresserError> Parser::parseScanStatement(Function &function) {
        //<scan-statement> ::=
        //    'scan' '(' <identifier> ')' ';'
        return {};
    }

    std::optional<ExpresserError> Parser::parseAssignmentExpression(Function &function) {
        //<assignment-expression>';'
        //<assignment-expression> ::=
        //    <identifier><assignment-operator><expression>
        // 如果类型是char，则隐式转换
        auto function_name = std::get<std::string>(_global_constants[function._name_index]._value);
        auto token = nextToken();
        auto var_name = token->GetStringValue();
        TokenType var_type;

        if (isConstant(function_name, var_name))
            return errorFactory(ErrorCode::ErrAssignToConstant);

        if (isLocalVariable(function_name, var_name)) {
            var_type = getVariableType(function_name, var_name).value();
            auto index = function._instructions.size();
            auto var_index = getIndex(function_name, var_name).first.value();
            function._instructions.emplace_back(Instruction(index, Operation::LOADA, 2, 0, 4, var_index));
            // 移出未初始化区
            if (isLocalUnInitializedVariable(function_name, var_name)) {
                auto it = function._local_uninitialized.find(var_name);
                function._local_vars.insert({it->first, it->second});
                function._local_uninitialized.erase(var_name);
            }
        } else if (isGlobalVariable(var_name)) {
            var_type = getVariableType(var_name).value();
            auto index = function._instructions.size();
            auto var_index = getIndex(var_name).first.value();
            function._instructions.emplace_back(Instruction(index, Operation::LOADA, 2, 1, 4, var_index));
            // 移出未初始化区
            if (isGlobalUnInitializedVariable(var_name)) {
                auto it = _global_uninitialized.find(var_name);
                _global_variables.insert({it->first, it->second});
                _global_uninitialized.erase(var_name);
            }
        } else {
            return errorFactory(ErrorCode::ErrUndeclaredIdentifier);
        }

        token = nextToken();
        if (!token.has_value() || token->GetType() != ASSIGN)
            return errorFactory(ErrorCode::ErrInvalidAssignment);

        auto err = parseExpression(var_type, function);
        if (err.has_value())
            return err.value();

        // TODO
        token = nextToken();
        if (!token.has_value() || token->GetType() != SEMICOLON)
            return errorFactory(ErrorCode::ErrNoSemicolon);

        auto index = function._instructions.size();
        function._instructions.emplace_back(Instruction(index, Operation::ISTORE));

        return {};
    }

    std::optional<ExpresserError> Parser::parseExpression(TokenType type, std::optional<Function> function) {
        // TODO: impl it
        //<expression> ::=
        //    <additive-expression>
        //<additive-expression> ::=
        //     <multiplicative-expression>{<additive-operator><multiplicative-expression>}
        return {};
    }

    std::optional<ExpresserError> Parser::parseMultiplicativeExpression(TokenType type, std::optional<Function> function) {
        // TODO: impl it
        // 拓展C0，类型转换
        //<multiplicative-expression> ::=
        //     <cast-expression>{<multiplicative-operator><cast-expression>}
        return {};
    }

    std::optional<ExpresserError> Parser::parseUnaryExpression(TokenType type, std::optional<Function> function) {
        // TODO: impl it
        //<unary-expression> ::=
        //    [<unary-operator>]<primary-expression>
        return {};
    }

    std::optional<ExpresserError> Parser::parsePrimaryExpression(TokenType type, std::optional<Function> function) {
        // TODO: impl it
        //<primary-expression> ::=
        //     '('<expression>')'
        //    |<identifier>
        //    |<integer-literal>
        //    |<function-call>
        //    |<char-literal>   --   拓展C0，char
        return {};
    }

    std::optional<ExpresserError> Parser::parseFunctionCall(Function &function) {
        //<function-call> ::=
        //    <identifier> '(' [<expression-list>] ')'
        //<expression-list> ::=
        //    <expression>{','<expression>}
        return {};
    }

    std::optional<ExpresserError> Parser::parseCastExpression(Function &function) {
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