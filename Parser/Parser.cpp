#include "Parser/Parser.h"

#include <utility>

namespace expresser {
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

    std::optional<Token> Parser::nextToken() {
        if (_offset == _tokens.size())
            return {};
        _current_pos = _tokens[_offset].GetEndPos();
        return _tokens[_offset++];
    }

    std::optional<Token> Parser::seekToken(int32_t offset) {
        if (_offset + offset - 1 >= _tokens.size())
            return {};
        return _tokens[_offset + offset - 1];
    }

    void Parser::rollback() {
        if (_offset <= 0) {
            std::cerr << "Cannot rollback" << std::endl;
            exit(2);
        }
        _offset--;
    }

    template<typename T>
    std::optional<ExpresserError> Parser::addGlobalConstant(const std::string &constant_name, const char type, T value) {
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
        _global_stack_constants.insert({variable_name, _global_sp++});
        _global_type_map.insert({variable_name, type});
        return {};
    }

    std::optional<ExpresserError>
    Parser::addGlobalVariable(const std::string &variable_name, TokenType type) {
        if (isGlobalVariable(variable_name))
            return errorFactory(ErrorCode::ErrDuplicateDeclaration);
        // 全局堆栈上分配全局变量
        // 无初始值，用snew
        auto index = _start_instruments.size();
        _start_instruments.emplace_back(Instruction(index, Operation::SNEW, 4, 1));
        _global_uninitialized.insert({variable_name, _global_sp++});
        _global_type_map.insert({variable_name, type});
        return {};
    }

    std::pair<Function *, std::optional<ExpresserError>>
    Parser::addFunction(const std::string &function_name, const TokenType &return_type, const std::vector<FunctionParam> &params) {
        auto err = addGlobalConstant(function_name, 'S', function_name);
        if (err.has_value())
            return std::make_pair(nullptr, err.value());
        Function function(_functions.size(), _global_constants_index.find(function_name)->second, params.size(), return_type, params);
        // 把参数信息放入函数变量、常量表中
        for (int32_t i = 0; i < params.size(); i++) {
            auto p = params[i];
            if (p._is_const)
                function._local_constants.insert({p._value, i});
            else
                function._local_vars.insert({p._value, i});
            function._local_type_map.insert({p._value, p._type});
        }
        _functions[function_name] = function;
        return std::make_pair(&_functions[function_name], std::optional<ExpresserError>());
    }

    std::optional<ExpresserError>
    Parser::addLocalConstant(Function &function, TokenType type, const std::string &constant_name) {
        if (isLocalVariable(function, constant_name))
            return errorFactory(ErrorCode::ErrDuplicateDeclaration);
        // 局部堆栈上分配局部常量
        auto index = function._instructions.size();
        function._instructions.emplace_back(Instruction(index, Operation::SNEW, 4, 1));
        function._local_constants.insert({constant_name, function._local_sp++});
        function._local_type_map.insert({constant_name, type});
        return {};
    }

    std::optional<ExpresserError>
    Parser::addLocalVariable(Function &function, TokenType type, const std::string &variable_name) {
        if (isLocalVariable(function, variable_name))
            return errorFactory(ErrorCode::ErrDuplicateDeclaration);
        // 局部堆栈上分配局部变量
        // 无初始值，用snew
        auto index = function._instructions.size();
        function._instructions.emplace_back(Instruction(index, Operation::SNEW, 4, 1));
        function._local_uninitialized.insert({variable_name, function._local_sp++});
        function._local_type_map.insert({variable_name, type});
        return {};
    }

    std::pair<std::optional<int32_t>, std::optional<ExpresserError>>
    Parser::getIndex(const std::string &variable_name) {
        int32_t res = -1;
        if (isGlobalVariable(variable_name)) {
            if (_global_constants_index.find(variable_name) != _global_constants_index.end())
                res = _global_constants_index[variable_name];
            else if (_global_stack_constants.find(variable_name) != _global_stack_constants.end())
                res = _global_stack_constants[variable_name];
            else if (_global_uninitialized.find(variable_name) != _global_uninitialized.end())
                res = _global_uninitialized[variable_name];
            else
                res = _global_variables[variable_name];
        }
        if (res != -1)
            return std::make_pair(std::make_optional<int32_t>(res), std::optional<ExpresserError>());
        return std::make_pair(std::optional<int32_t>(), errorFactory(ErrorCode::ErrUndeclaredIdentifier));
    }

    std::pair<std::optional<int32_t>, std::optional<ExpresserError>> Parser::getIndex(Function &function, const std::string &variable_name) {
        // 先在本函数的变量区、常量区找，后在全局常量区找
        int32_t res = -1;
        if (isLocalVariable(function, variable_name)) {
            if (function._local_constants.find(variable_name) != function._local_constants.end())
                res = function._local_constants[variable_name];
            else if (function._local_uninitialized.find(variable_name) != function._local_uninitialized.end())
                res = function._local_uninitialized[variable_name];
            else
                res = function._local_vars[variable_name];
        } else if (isGlobalVariable(variable_name)) {
            return getIndex(variable_name);
        }
        if (res != -1)
            return std::make_pair(std::make_optional<int32_t>(res), std::optional<ExpresserError>());
        return std::make_pair(std::optional<int32_t>(), errorFactory(ErrorCode::ErrUndeclaredIdentifier));
    }

    std::pair<Function *, std::optional<ExpresserError>> Parser::getFunction(const std::string &function_name) {
        auto it = _functions.find(function_name);
        if (it == _functions.end())
            return std::make_pair(nullptr, errorFactory(ErrorCode::ErrUndeclaredFunction));
        return std::make_pair(&it->second, std::optional<ExpresserError>());
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

    bool Parser::isLocalVariable(Function &function, const std::string &variable_name) {
        return isLocalInitializedVariable(function, variable_name) ||
               isLocalUnInitializedVariable(function, variable_name) ||
               isLocalConstant(function, variable_name);
    }

    bool Parser::isLocalInitializedVariable(Function &function, const std::string &variable_name) {
        return function._local_vars.find(variable_name) != function._local_vars.end();
    }

    bool Parser::isLocalUnInitializedVariable(Function &function, const std::string &variable_name) {
        return function._local_uninitialized.find(variable_name) != function._local_uninitialized.end();
    }

    bool Parser::isLocalConstant(Function &function, const std::string &variable_name) {
        return function._local_constants.find(variable_name) != function._local_constants.end();
    }

    bool Parser::isInitialized(Function &function, const std::string &variable_name) {
        return isLocalInitializedVariable(function, variable_name) || isGlobalInitializedVariable(variable_name);
    }

    bool Parser::isUnInitialized(Function &function, const std::string &variable_name) {
        return isLocalUnInitializedVariable(function, variable_name) && isGlobalUnInitializedVariable(variable_name);
    }

    bool Parser::isConstant(Function &function, const std::string &variable_name) {
        return isLocalConstant(function, variable_name) || isGlobalConstant(variable_name);
    }

    std::optional<TokenType> Parser::getVariableType(const std::string &variable_name) {
        auto it = _global_type_map.find(variable_name);
        if (it != _global_type_map.end())
            return it->second;
        return std::optional<TokenType>();
    }

    std::optional<TokenType> Parser::getVariableType(Function &function, const std::string &variable_name) {
        auto it = function._local_type_map.find(variable_name);
        if (it != function._local_type_map.end())
            return it->second;
        return std::optional<TokenType>();
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
                    auto res = parseExpression(nullptr);
                    if (res.second.has_value())
                        return res.second.value();

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
                    addGlobalVariable(identifier, var_type);

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
                        auto res = parseExpression({});
                        if (res.second.has_value())
                            return res.second.value();

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
            } else {
                // 可能是函数声明，先不报错
                rollback();
                return {};
            }
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseFunctionDefinitions() {
        // 1个或无数个
        for (; !_program_end;) {
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
        if (!token.has_value()) {
            _program_end = true;
            return {};
        }
        if (token->GetType() != TokenType::RESERVED || !isFunctionReturnType(token.value()))
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
        auto res = addFunction(function_name, return_type, params);
        if (res.second.has_value())
            return res.second.value();

        // <compound-statement>
        auto function = res.first;
        err = parseCompoundStatement(*function);
        if (err.has_value())
            return err.value();

        // 如果是void函数，添加ret
        if (return_type == VOID) {
            auto index = function->_instructions.size();
            function->_instructions.emplace_back(Instruction(index, Operation::RET));
        }

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
        auto seek = seekToken(1);
        if (seek.has_value() && seek->GetType() == RIGHTBRACKET) {
            // 无参数
            nextToken();
            return {};
        }
        for (;; token = nextToken()) {
            if (!token.has_value())
                return errorFactory(ErrorCode::ErrEOF);
            else if (token->GetType() == RIGHTBRACKET)
                // 右括号，退出
                break;
            else if (token->GetType() == RESERVED &&
                     (isVariableType(token.value()) || token->GetStringValue() == "const")) {
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
        //<variable-declaration> ::=
        //    [<const-qualifier>]<type-specifier><init-declarator-list>';'
        //<init-declarator-list> ::=
        //    <init-declarator>{','<init-declarator>}
        //<init-declarator> ::=
        //    <identifier>[<initializer>]
        //<initializer> ::=
        //    '='<expression>
        // seekToken预读
        for (;;) {
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
                    if (isLocalVariable(function, identifier))
                        return errorFactory(ErrorCode::ErrDuplicateDeclaration);
                    addLocalConstant(function, const_type, identifier);

                    token = nextToken();
                    if (!token.has_value() || token->GetType() != ASSIGN)
                        return errorFactory(ErrorCode::ErrNeedAssignSymbol);

                    auto const_index = getIndex(function, identifier).first.value();
                    auto index = function._instructions.size();
                    function._instructions.emplace_back(Instruction(index, Operation::LOADA, 2, 0, 4, const_index));

                    auto res = parseExpression(&function);
                    if (res.second.has_value())
                        return res.second.value();

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
            } else if (token->GetType() == RESERVED && isVariableType(token.value())) {
                // var or func
                // 预读判断函数
                auto seek = seekToken(2);
                if (seek.has_value() && seek->GetType() == LEFTBRACKET) {
                    // 回滚一个token
                    rollback();
                    return {};
                }
                auto res = stringTypeToTokenType(token->GetStringValue());
                if (!res.has_value())
                    return errorFactory(ErrorCode::ErrInvalidVariableType);
                TokenType var_type = res.value();
                for (;;) {
                    token = nextToken();
                    if (!token.has_value() || token->GetType() != IDENTIFIER)
                        return errorFactory(ErrorCode::ErrNeedIdentifier);
                    auto identifier = token->GetStringValue();
                    if (isLocalVariable(function, identifier))
                        return errorFactory(ErrorCode::ErrDuplicateDeclaration);
                    addLocalVariable(function, var_type, identifier);

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

                        auto res = parseExpression(&function);
                        if (res.second.has_value())
                            return res.second.value();

                        function._instructions.emplace_back(Instruction(index + 1, Operation::ISTORE));
                        auto it = function._local_uninitialized.find(identifier);
                        if (it != function._local_uninitialized.end()) {
                            function._local_vars.insert({it->first, it->second});
                            function._local_uninitialized.erase(identifier);
                        }

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
            } else {
                rollback();
                return {};
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
        for (;;) {
            auto err = parseStatement(function);
            if (err.second.has_value())
                return err.second.value();
            if (err.first)
                break;
        }
        return {};
    }

    std::pair<bool, std::optional<ExpresserError>> Parser::parseStatement(Function &function) {
        bool statement_end = false;
        auto token = nextToken();
        if (!token.has_value())
            return std::make_pair(statement_end, errorFactory(ErrorCode::ErrEOF));
        if (token->GetType() == RIGHTBRACE) {
            // 右括号，函数结束
            rollback();
            statement_end = true;
        } else if (token->GetType() == LEFTBRACE) {
            auto err = parseStatements(function);
            if (err.has_value())
                return std::make_pair(statement_end, err.value());
            token = nextToken();
            if (!token.has_value() || token->GetType() != RIGHTBRACE)
                return std::make_pair(statement_end, errorFactory(ErrorCode::ErrMissingBrace));
        } else if (token->GetType() == RESERVED && token->GetStringValue() == "if") {
            rollback();
            auto err = parseConditionStatement(function);
            if (err.has_value())
                return std::make_pair(statement_end, err.value());
        } else if (token->GetType() == RESERVED &&
                   (token->GetStringValue() == "do" || token->GetStringValue() == "while")) {
            rollback();
            auto err = parseLoopStatement(function);
            if (err.has_value())
                return std::make_pair(statement_end, err.value());
        } else if (token->GetType() == RESERVED &&
                   (token->GetStringValue() == "return" ||
                    token->GetStringValue() == "continue" ||
                    token->GetStringValue() == "break")) {
            rollback();
            auto err = parseJumpStatement(function);
            if (err.has_value())
                return std::make_pair(statement_end, err.value());
        } else if (token->GetType() == RESERVED && token->GetStringValue() == "print") {
            rollback();
            auto err = parsePrintStatement(function);
            if (err.has_value())
                return std::make_pair(statement_end, err.value());
        } else if (token->GetType() == RESERVED && token->GetStringValue() == "scan") {
            rollback();
            auto err = parseScanStatement(function);
            if (err.has_value())
                return std::make_pair(statement_end, err.value());
        } else if (token->GetType() == IDENTIFIER) {
            // function call or assign
            auto seek = seekToken(1);
            std::optional<ExpresserError> err;
            if (!seek.has_value())
                return std::make_pair(statement_end, errorFactory(ErrorCode::ErrEOF));
            rollback();
            if (seek->GetType() == LEFTBRACKET) {
                auto res = parseFunctionCall(&function);
                err = res.second;
            } else {
                err = parseAssignmentExpression(function);
            }
            if (err.has_value())
                return std::make_pair(statement_end, err.value());
        } else if (token->GetType() == SEMICOLON)
            statement_end = false;
        else
            return std::make_pair(statement_end, errorFactory(ErrorCode::ErrInvalidStatement));
        return std::make_pair(statement_end, std::optional<ExpresserError>());
    }

    std::optional<ExpresserError> Parser::parseConditionStatement(Function &function) {
        //<condition> ::=
        //     <expression>[<relational-operator><expression>]
        //<condition-statement> ::=
        //     'if' '(' <condition> ')' <statement> ['else' <statement>]
        // | ------------------ |
        // |    condition-lhs   |
        // |    condition-rhs   |
        // |       compare      |
        // |    jmp-1->nop-2    | -> 条件不符合时跳转
        // | ------------------ |
        // |     <if-block>     |
        // | nop-1/jmp-2->nop-3 | -> 有else块时无条件跳转
        // |        nop-2       |
        // | ------------------ | -> 可能不存在
        // |    <else-block>    |
        // |        nop-3       |
        // | ------------------ |
        // 跳过'if'
        nextToken();
        // (
        auto token = nextToken();
        if (!token.has_value() || token->GetType() != LEFTBRACKET)
            return errorFactory(ErrorCode::ErrMissingBracket);

        // <condition>
        auto cond = parseCondition(function);
        if (cond.second.has_value())
            return cond.second.value();
        Operation operation = cond.first.value();
        token = nextToken();

        // )
        if (!token.has_value() || token->GetType() != RIGHTBRACKET)
            return errorFactory(ErrorCode::ErrMissingBracket);

        // 预留位置，待<statement>解析完毕后修改
        auto jmp_index = function._instructions.size();
        function._instructions.emplace_back(jmp_index, Operation::NOP);

        // <statement>
        // if块
        auto err = parseStatement(function);
        if (err.second.has_value())
            return err.second.value();

        // if块末尾添加两个nop
        // 第一个nop用于有else时跳过else块
        // 第二个nop用于条件不符合时跳入else块
        // 条件不符合时JMP目标的位置
        auto nop_index = function._instructions.size();
        // NOP1
        function._instructions.emplace_back(nop_index, Operation::NOP);
        // NOP2
        function._instructions.emplace_back(nop_index + 1, Operation::NOP);
        // 修改原来的JMP
        function._instructions[jmp_index] = Instruction(jmp_index, operation, 2, nop_index + 1);

        // 预读,else块
        auto seek = seekToken(1);
        if (seek.has_value() && seek->GetType() == RESERVED && seek->GetStringValue() == "else") {
            nextToken();
            err = parseStatement(function);
            if (err.second.has_value())
                return err.second.value();
            auto index = function._instructions.size();
            // 修改if块末尾第一个nop，防止执行到else块
            // NOP3
            function._instructions.emplace_back(Instruction(index, Operation::NOP));
            function._instructions[nop_index] = Instruction(index + 1, Operation::JMP, 2, nop_index);
        }
        return {};
    }

    std::pair<std::optional<Operation>, std::optional<ExpresserError>> Parser::parseCondition(Function &function) {
        //<condition> ::=
        //    <expression>[<relational-operator><expression>]
        Operation operation;
        // LHS
        auto res = parseExpression(&function);
        if (res.second.has_value())
            return std::make_pair(std::optional<Operation>(), res.second.value());

        auto seek = seekToken(1);
        if (seek.has_value() && seek->GetType() == RIGHTBRACKET) {
            // LHS==0 跳过if块
            operation = Operation::JE;
            // CODE
            auto index = function._instructions.size();
            function._instructions.emplace_back(Instruction(index, Operation::IPUSH, 4, 0));
            function._instructions.emplace_back(Instruction(index + 1, Operation::ICMP));
            return std::make_pair(std::make_optional<Operation>(operation), std::optional<ExpresserError>());
        } else {
            // 关系符号
            auto token = nextToken();
            if (!token.has_value() || relation_token_set.find(token->GetType()) == relation_token_set.end())
                return std::make_pair(std::optional<Operation>(), errorFactory(ErrorCode::ErrNeedRelationalOperator));
            TokenType relation = token->GetType();
            operation = if_jmp_map.find(relation)->second;

            // RHS
            res = parseExpression(&function);
            if (res.second.has_value())
                return std::make_pair(std::optional<Operation>(), res.second.value());

            // CODE
            auto index = function._instructions.size();
            function._instructions.emplace_back(Instruction(index, Operation::ICMP));
            return std::make_pair(std::make_optional<Operation>(operation), std::optional<ExpresserError>());
        }
    }

    std::optional<ExpresserError> Parser::parseLoopStatement(Function &function) {
        //<loop-statement> ::=
        //    'while' '(' <condition> ')' <statement>
        //   |'do' <statement> 'while' '(' <condition> ')' ';'   --   拓展C0，`do...while`
        // break和continue
        int32_t continue_index, break_index;
        // 循环嵌套，保存上一个循环的跳转信息
        std::vector<std::pair<int32_t, std::string>> prev_loop;
        prev_loop.assign(function._loop_jumps.begin(), function._loop_jumps.end());
        // 清空之前的跳转信息
        function._loop_jumps.clear();
        auto token = nextToken();
        if (token->GetStringValue() == "do") {
            // do...while
            // | --------------- |
            // |      nop-1      | -> 用于正常循环和continue
            // | statement-block |
            // | --------------- |
            // |       lhs       |
            // |       rhs       |
            // |       cmp       |
            // |    jmp->nop-1   | -> 条件符合时跳转
            // |      nop-2      | -> 用于break跳出
            // | --------------- |
            auto nop_index = function._instructions.size();
            function._instructions.emplace_back(Instruction(nop_index, Operation::NOP));
            auto err = parseStatement(function);
            if (err.second.has_value())
                return err.second.value();
            token = nextToken();
            if (!token.has_value() || token->GetStringValue() != "while")
                return errorFactory(ErrorCode::ErrInvalidLoop);
            token = nextToken();
            if (!token.has_value() || token->GetType() != LEFTBRACKET)
                return errorFactory(ErrorCode::ErrMissingBracket);
            auto res = parseCondition(function);
            if (res.second.has_value())
                return res.second.value();
            auto operation = res.first.value();
            token = nextToken();
            if (!token.has_value() || token->GetType() != RIGHTBRACKET)
                return errorFactory(ErrorCode::ErrMissingBracket);
            token = nextToken();
            if (!token.has_value() || token->GetType() != SEMICOLON)
                return errorFactory(ErrorCode::ErrNoSemicolon);
            // 返回的是条件条件不符合时的跳转命令
            // 再反转一次
            operation = reverse_map.find(operation)->second;
            auto index = function._instructions.size();
            function._instructions.emplace_back(Instruction(index, operation, 2, nop_index));
            function._instructions.emplace_back(Instruction(index + 1, Operation::NOP));
            continue_index = nop_index;
            break_index = index + 1;
        } else if (token->GetStringValue() == "while") {
            // while
            // | --------------- |
            // |       nop-1     | -> 用于正常循环和continue
            // |       lhs       |
            // |       rhs       |
            // |       cmp       |
            // |  jmp-1->nop-3   | -> 条件不符合时跳转
            // | --------------- |
            // | statement-block |
            // |  jmp-2->nop-1   | -> 无条件跳转
            // | --------------- |
            // |      nop-3      | -> 用于跳出和break
            // | --------------- |
            token = nextToken();
            if (!token.has_value() || token->GetType() != LEFTBRACKET)
                return errorFactory(ErrorCode::ErrMissingBracket);
            auto nop1_index = function._instructions.size();
            function._instructions.emplace_back(Instruction(nop1_index, Operation::NOP));
            auto res = parseCondition(function);
            if (res.second.has_value())
                return res.second.value();
            auto operation = res.first.value();
            token = nextToken();
            if (!token.has_value() || token->GetType() != RIGHTBRACKET)
                return errorFactory(ErrorCode::ErrMissingBracket);
            auto nop2_index = function._instructions.size();
            function._instructions.emplace_back(Instruction(nop2_index, Operation::NOP));
            auto err = parseStatement(function);
            if (err.second.has_value())
                return err.second.value();
            auto index = function._instructions.size();
            function._instructions.emplace_back(Instruction(index, Operation::JMP, 2, nop1_index));
            function._instructions.emplace_back(Instruction(index + 1, Operation::NOP));
            auto nop3_index = function._instructions.size();
            // 修改跳转地址
            function._instructions[nop2_index] = Instruction(nop2_index, operation, 2, nop3_index);
            continue_index = nop1_index;
            break_index = nop3_index;
        } else {
            return errorFactory(ErrorCode::ErrInvalidLoop);
        }
        // 处理内部break和continue
        for (const auto &jump:function._loop_jumps) {
            if (jump.second == "break")
                function._instructions[jump.first] = Instruction(jump.first, Operation::JMP, 2, break_index);
            else if (jump.second == "continue")
                function._instructions[jump.first] = Instruction(jump.first, Operation::JMP, 2, continue_index);
            else
                return errorFactory(ErrorCode::ErrInvalidJump);
        }
        // 清空跳转信息
        function._loop_jumps.clear();
        // 恢复上一个循环的跳转信息
        function._loop_jumps.assign(prev_loop.begin(), prev_loop.end());
        return {};
    }

    std::optional<ExpresserError> Parser::parseJumpStatement(Function &function) {
        //<jump-statement> ::=
        //     'break' ';'
        //    |'continue' ';'
        //    |<return-statement>
        //<return-statement> ::=
        //    'return' [<expression>] ';'
        // 先跳过'return'
        auto token = nextToken();
        if (token->GetStringValue() == "return") {
            auto return_type = function._return_type;
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
            auto res = parseExpression(&function);
            if (res.second.has_value())
                return res.second.value();
            auto index = function._instructions.size();
            function._instructions.emplace_back(Instruction(index, Operation::IRET));
        } else if (token->GetStringValue() == "break" || token->GetStringValue() == "continue") {
            auto index = function._instructions.size();
            function._instructions.emplace_back(Instruction(index, Operation::NOP));
            function._loop_jumps.emplace_back(std::make_pair(index, token->GetStringValue()));
        } else {
            return errorFactory(ErrorCode::ErrInvalidStatement);
        }
        // ;
        token = nextToken();
        if (!token.has_value() || token->GetType() != SEMICOLON)
            return errorFactory(ErrorCode::ErrNoSemicolon);
        return {};
    }

    std::optional<ExpresserError> Parser::parsePrintStatement(Function &function) {
        //<print-statement> ::=
        //    'print' '(' [<printable-list>] ')' ';'
        // 跳过'print'
        nextToken();
        // '('
        auto token = nextToken();
        if (!token.has_value() || token->GetType() != LEFTBRACKET)
            return errorFactory(ErrorCode::ErrMissingBracket);
        // [<printable-list>]
        auto err = parsePrintableList(function);
        if (err.has_value())
            return err.value();
        // ')'
        token = nextToken();
        if (!token.has_value() || token->GetType() != RIGHTBRACKET)
            return errorFactory(ErrorCode::ErrMissingBracket);
        // ';'
        token = nextToken();
        if (!token.has_value() || token->GetType() != SEMICOLON)
            return errorFactory(ErrorCode::ErrNoSemicolon);
        // 换行
        auto index = function._instructions.size();
        function._instructions.emplace_back(Instruction(index, Operation::PRINTL));
        return {};
    }

    std::optional<ExpresserError> Parser::parsePrintableList(Function &function) {
        //<printable-list>  ::=
        //    <printable> {',' <printable>}
        //<printable> ::=
        //    <expression> | <string-literal> | <char-literal>   --   拓展C0，字面量打印
        std::optional<Token> token;
        for (;;) {
            token = nextToken();
            if (!token.has_value())
                return errorFactory(ErrorCode::ErrInvalidPrint);
            if (token->GetType() == RIGHTBRACKET) {
                rollback();
                break;
            } else if (token->GetType() == COMMA)
                continue;
            else if (token->GetType() == CHARLITERAL) {
                auto index = function._instructions.size();
                function._instructions.emplace_back(
                        Instruction(index, Operation::IPUSH, 4, std::any_cast<int32_t>(token->GetValue())));
                function._instructions.emplace_back(Instruction(index + 1, Operation::CPRINT));
            } else if (token->GetType() == STRINGLITERAL) {
                auto const_index = _global_constants.size();
                _global_constants.emplace_back(Constant(const_index, 'S', token->GetStringValue()));
                auto index = function._instructions.size();
                function._instructions.emplace_back(Instruction(index, Operation::LOADC, 2, const_index));
                function._instructions.emplace_back(Instruction(index + 1, Operation::SPRINT));
            } else {
                rollback();
                auto res = parseExpression(&function);
                if (res.second.has_value())
                    return res.second.value();
                // 根据结果类型调用iprint/cprint
                if (!res.first.has_value())
                    return errorFactory(ErrorCode::ErrInvalidVariableType);
                auto type = res.first.value();
                auto index = function._instructions.size();
                if (type == CHARLITERAL)
                    function._instructions.emplace_back(Instruction(index, Operation::CPRINT));
                else if (type == INTEGER)
                    function._instructions.emplace_back(Instruction(index, Operation::IPRINT));
                else
                    return errorFactory(ErrorCode::ErrInvalidPrint);
            }
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseScanStatement(Function &function) {
        //<scan-statement> ::=
        //    'scan' '(' <identifier> ')' ';'
        int32_t index, var_index, level;
        TokenType type;
        // 跳过'scan'
        nextToken();
        // '('
        auto token = nextToken();
        if (!token.has_value() || token->GetType() != LEFTBRACKET)
            return errorFactory(ErrorCode::ErrMissingBracket);
        // <identifier>
        token = nextToken();
        auto function_name = std::get<std::string>(_global_constants[function._name_index]._value);
        if (!token.has_value())
            return errorFactory(ErrorCode::ErrInvalidScan);
        auto identifier = token->GetStringValue();
        if (isConstant(function, identifier))
            return errorFactory(ErrorCode::ErrAssignToConstant);
        if (isLocalVariable(function, identifier)) {
            var_index = getIndex(function, identifier).first.value();
            index = function._instructions.size();
            level = 0;
            auto res = getVariableType(function, identifier);
            if (!res.has_value())
                return errorFactory(ErrorCode::ErrInvalidVariableType);
            type = res.value();
            if (isLocalUnInitializedVariable(function, identifier)) {
                auto it = function._local_uninitialized.find(identifier);
                function._local_vars.insert({it->first, it->second});
                function._local_uninitialized.erase(identifier);
            }
        } else if (isGlobalVariable(identifier)) {
            var_index = getIndex(identifier).first.value();
            index = function._instructions.size();
            level = 1;
            auto res = getVariableType(identifier);
            if (!res.has_value())
                return errorFactory(ErrorCode::ErrInvalidVariableType);
            type = res.value();
            if (isGlobalUnInitializedVariable(identifier)) {
                auto it = _global_uninitialized.find(identifier);
                _global_variables.insert({it->first, it->second});
                _global_uninitialized.erase(identifier);
            }
        } else {
            return errorFactory(ErrorCode::ErrUndeclaredIdentifier);
        }
        function._instructions.emplace_back(Instruction(index, Operation::LOADA, 2, level, 4, var_index));
        // ')'
        token = nextToken();
        if (!token.has_value() || token->GetType() != RIGHTBRACKET)
            return errorFactory(ErrorCode::ErrMissingBracket);
        // ';'
        token = nextToken();
        if (!token.has_value() || token->GetType() != SEMICOLON)
            return errorFactory(ErrorCode::ErrNoSemicolon);

        if (type == CHARLITERAL)
            function._instructions.emplace_back(Instruction(index + 1, Operation::CSCAN));
        else if (type == INTEGER)
            function._instructions.emplace_back(Instruction(index + 1, Operation::ISCAN));
        else
            return errorFactory(ErrorCode::ErrInvalidVariableType);
        function._instructions.emplace_back(Instruction(index + 2, Operation::ISTORE));
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

        if (isConstant(function, var_name))
            return errorFactory(ErrorCode::ErrAssignToConstant);

        if (isLocalVariable(function, var_name)) {
            var_type = getVariableType(function, var_name).value();
            auto index = function._instructions.size();
            auto var_index = getIndex(function, var_name).first.value();
            function._instructions.emplace_back(Instruction(index, Operation::LOADA, 2, 0, 4, var_index));
            // 移出未初始化区
            if (isLocalUnInitializedVariable(function, var_name)) {
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

        auto res = parseExpression(&function);
        if (res.second.has_value())
            return res.second.value();
        auto res_type = res.first.value();
        if (var_type == CHARLITERAL && res_type == INTEGER) {
            auto index = function._instructions.size();
            function._instructions.emplace_back(Instruction(index, Operation::I2C));
        }

        token = nextToken();
        if (!token.has_value() || token->GetType() != SEMICOLON)
            return errorFactory(ErrorCode::ErrNoSemicolon);

        auto index = function._instructions.size();
        function._instructions.emplace_back(Instruction(index, Operation::ISTORE));

        return {};
    }

    std::pair<std::optional<TokenType>, std::optional<ExpresserError>> Parser::parseExpression(Function *function) {
        //<expression> ::=
        //    <additive-expression>
        //<additive-expression> ::=
        //     <multiplicative-expression>{<additive-operator><multiplicative-expression>}
        bool has_rhs = false;
        TokenType lhs_type = INTEGER, rhs_type = INTEGER, return_type = INTEGER;
        auto res = parseMultiplicativeExpression(function);
        if (res.second.has_value())
            return std::make_pair(std::optional<TokenType>(), res.second.value());
        lhs_type = res.first.value();

        auto seek = seekToken(1);
        if (seek.has_value() && (seek->GetType() == PLUS || seek->GetType() == MINUS)) {
            // 跳过
            nextToken();
            Operation operation;
            operation = seek->GetType() == PLUS ? Operation::IADD : Operation::ISUB;

            res = parseMultiplicativeExpression(function);
            if (res.second.has_value())
                return std::make_pair(std::optional<TokenType>(), res.second.value());
            rhs_type = res.first.value();
            has_rhs = true;

            std::vector<Instruction> *_instruction_vector;
            if (function != nullptr)
                _instruction_vector = &function->_instructions;
            else
                _instruction_vector = &_start_instruments;
            auto index = _instruction_vector->size();
            _instruction_vector->emplace_back(Instruction(index, operation));
        }
        if (!has_rhs)
            return_type = lhs_type;
        else {
            if (lhs_type == rhs_type)
                return_type = lhs_type;
            else
                return_type = INTEGER;
        }
        return std::make_pair(return_type, std::optional<ExpresserError>());
    }

    std::pair<std::optional<TokenType>, std::optional<ExpresserError>> Parser::parseMultiplicativeExpression(Function *function) {
        // 拓展C0，类型转换
        //<multiplicative-expression> ::=
        //     <cast-expression>{<multiplicative-operator><cast-expression>}
        bool has_rhs = false;
        TokenType lhs_type = INTEGER, rhs_type = INTEGER, return_type = INTEGER;
        auto res = parseCastExpression(function);
        if (res.second.has_value())
            return std::make_pair(std::optional<TokenType>(), res.second.value());
        lhs_type = res.first.value();

        auto seek = seekToken(1);
        if (seek.has_value() && (seek->GetType() == MULTIPLE || seek->GetType() == DIVIDE)) {
            // 跳过
            nextToken();
            Operation operation;
            operation = seek->GetType() == MULTIPLE ? Operation::IMUL : Operation::IDIV;

            res = parseCastExpression(function);
            if (res.second.has_value())
                return std::make_pair(std::optional<TokenType>(), res.second.value());
            rhs_type = res.first.value();
            has_rhs = true;

            std::vector<Instruction> *_instruction_vector;
            if (function != nullptr)
                _instruction_vector = &function->_instructions;
            else
                _instruction_vector = &_start_instruments;
            auto index = _instruction_vector->size();
            _instruction_vector->emplace_back(Instruction(index, operation));
        }
        if (!has_rhs)
            return_type = lhs_type;
        else {
            if (lhs_type == rhs_type)
                return_type = lhs_type;
            else
                return_type = INTEGER;
        }
        return std::make_pair(return_type, std::optional<ExpresserError>());
    }

    std::pair<std::optional<TokenType>, std::optional<ExpresserError>>
    Parser::parseUnaryExpression(Function *function) {
        //<unary-expression> ::=
        //    [<unary-operator>]<primary-expression>
        TokenType return_type = INTEGER;
        bool reverse = false;
        auto token = nextToken();
        if (!token.has_value())
            return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrIncompleteExpression));
        if (token->GetType() == PLUS)
            reverse = false;
        else if (token->GetType() == MINUS)
            reverse = true;
        else
            rollback();

        auto res = parsePrimaryExpression(function);
        if (res.second.has_value())
            return std::make_pair(std::optional<TokenType>(), res.second.value());
        return_type = res.first.value();

        if (reverse) {
            std::vector<Instruction> *_instruction_vector;
            if (function != nullptr)
                _instruction_vector = &function->_instructions;
            else
                _instruction_vector = &_start_instruments;
            auto index = _instruction_vector->size();
            _instruction_vector->emplace_back(Instruction(index, Operation::INEG));
        }
        return std::make_pair(return_type, std::optional<ExpresserError>());
    }

    std::pair<std::optional<TokenType>, std::optional<ExpresserError>>
    Parser::parsePrimaryExpression(Function *function) {
        //<primary-expression> ::=
        //     '('<expression>')'
        //    |<identifier>
        //    |<integer-literal>
        //    |<function-call>
        //    |<char-literal>   --   拓展C0，char
        auto token = nextToken();
        TokenType return_type = INTEGER;
        if (!token.has_value())
            return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrIncompleteExpression));
        switch (token->GetType()) {
            case LEFTBRACKET: {
                auto res = parseExpression(function);
                if (res.second.has_value())
                    return std::make_pair(std::optional<TokenType>(), res.second.value());
                return_type = res.first.value();
                break;
            }
            case CHARLITERAL:
            case INTEGER: {
                auto value = std::any_cast<int32_t>(token->GetValue());
                std::vector<Instruction> *_instruction_vector;
                if (function != nullptr)
                    _instruction_vector = &function->_instructions;
                else
                    _instruction_vector = &_start_instruments;
                auto index = _instruction_vector->size();
                _instruction_vector->emplace_back(Instruction(index, IPUSH, 4, value));
                return_type = token->GetType();
                break;
            }
            case IDENTIFIER: {
                auto seek = seekToken(1);
                if (!seek.has_value())
                    return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrIncompleteExpression));
                if (seek->GetType() == LEFTBRACKET) {
                    // CALL
                    rollback();
                    auto res = parseFunctionCall(function);
                    if (res.second.has_value())
                        return std::make_pair(std::optional<TokenType>(), res.second.value());
                    return_type = res.first.value();
                } else {
                    // IDENTIFIER
                    auto var_name = token->GetStringValue();
                    int32_t var_index;
                    if (function != nullptr) {
                        int32_t level;
                        auto index = function->_instructions.size();
                        auto function_name = std::get<std::string>(_global_constants[function->_name_index]._value);
                        if (isUnInitialized(*function, var_name))
                            return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrNotInitialized));
                        if (isLocalVariable(*function, var_name)) {
                            auto res = getIndex(*function, var_name);
                            if (res.second.has_value())
                                return std::make_pair(std::optional<TokenType>(), res.second.value());
                            var_index = res.first.value();
                            level = 0;
                            return_type = getVariableType(*function, var_name).value();
                        } else if (isGlobalVariable(var_name)) {
                            auto res = getIndex(var_name);
                            if (res.second.has_value())
                                return std::make_pair(std::optional<TokenType>(), res.second.value());
                            var_index = res.first.value();
                            level = 1;
                            return_type = getVariableType(var_name).value();
                        } else
                            return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrUndeclaredIdentifier));
                        function->_instructions.emplace_back(Instruction(index, LOADA, 2, level, 4, var_index));
                        function->_instructions.emplace_back(Instruction(index + 1, ILOAD));
                    } else {
                        auto index = _start_instruments.size();
                        if (isGlobalUnInitializedVariable(var_name))
                            return std::make_pair(std::optional<TokenType>(),
                                                  errorFactory(ErrorCode::ErrNotInitialized));
                        auto res = getIndex(var_name);
                        if (res.second.has_value())
                            return std::make_pair(std::optional<TokenType>(), res.second.value());
                        var_index = res.first.value();
                        _start_instruments.emplace_back(Instruction(index, LOADA, 2, 0, 4, var_index));
                        _start_instruments.emplace_back(Instruction(index + 1, ILOAD));
                        return_type = getVariableType(var_name).value();
                    }
                }
                break;
            }
            default:
                return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrInvalidExpression));
        }
        return std::make_pair(return_type, std::optional<ExpresserError>());
    }

    std::pair<std::optional<TokenType>, std::optional<ExpresserError>> Parser::parseFunctionCall(Function *function) {
        //<function-call> ::=
        //    <identifier> '(' [<expression-list>] ')'
        //<expression-list> ::=
        //    <expression>{','<expression>}
        // 无法在全局区调用函数
        TokenType return_type = VOID;
        if (function == nullptr)
            return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrInvalidFunctionCall));
        auto token = nextToken();
        if (!token.has_value() || token->GetType() != IDENTIFIER)
            return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrInvalidFunctionCall));
        auto function_name = token->GetStringValue();
        token = nextToken();
        if (!token.has_value() || token->GetType() != LEFTBRACKET)
            return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrMissingBracket));
        for (;;) {
            token = nextToken();
            if (!token.has_value())
                return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrInvalidFunctionCall));
            if (token->GetType() == COMMA)
                continue;
            else if (token->GetType() == RIGHTBRACKET)
                break;
            else {
                rollback();
                auto res = parseExpression(function);
                if (res.second.has_value())
                    return std::make_pair(std::optional<TokenType>(), res.second.value());
                return_type = res.first.value();
            }
        }
        auto index = function->_instructions.size();
        auto func_index = _functions[function_name]._index;
        function->_instructions.emplace_back(Instruction(index, Operation::CALL, 2, func_index));
        return std::make_pair(return_type, std::optional<ExpresserError>());
    }

    std::pair<std::optional<TokenType>, std::optional<ExpresserError>> Parser::parseCastExpression(Function *function) {
        // 拓展C0，类型转换
        //<cast-expression> ::=
        //    {'('<type-specifier>')'}<unary-expression>
        bool cast = false;
        TokenType return_type = INTEGER;
        auto seek1 = seekToken(1), seek2 = seekToken(2), seek3 = seekToken(3);
        if (seek1.has_value() && seek1->GetType() == LEFTBRACKET &&
            seek2.has_value() && seek2->GetType() == RESERVED &&
            seek3.has_value() && seek3->GetType() == RIGHTBRACKET) {
            // CAST
            auto cast_type = stringTypeToTokenType(seek2->GetStringValue());
            // int不用转，因为内部存储方式就是int
            // char只在print时需要强转
            // double不支持
            if (!cast_type.has_value())
                return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrInvalidCast));
            if (cast_type.value() == VOID)
                return std::make_pair(std::optional<TokenType>(), errorFactory(ErrorCode::ErrCastToVoid));
            // SKIP
            nextToken();
            nextToken();
            nextToken();
            cast = true;
            return_type = cast_type.value();
        }
        // UNARY
        auto res = parseUnaryExpression(function);
        if (res.second.has_value())
            return std::make_pair(std::optional<TokenType>(), res.second.value());
        if (!cast)
            return_type = res.first.value();
        return std::make_pair(return_type, std::optional<ExpresserError>());
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
        if (it != _function_return_type_map.end())
            return it->second;
        return {};
    }
}