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
        // TODO: 堆栈上分配全局变量
    }

    std::optional<ExpresserError> Parser::addFunction(const std::string &function_name, std::vector<FunctionParam> params) {
        auto err = addGlobalConstant(function_name, 'S', function_name);
        if (err.has_value())
            return err.value();

        Function func;
        func._index = _functions.size();
        func._name_index = _global_constants.find(function_name)->second._index;
        func._params_size = params.size();
        func._level = 1;
        func._params = params;

        _functions[function_name] = func;
        return {};
    }

    std::optional<ExpresserError> Parser::addInstrument(std::string function_name, Instruction instruction) {
        // TODO: instruction's factory
        auto err = getFunction(function_name);
        if (err.second.has_value())
            return err.second.value();
        auto func = err.first.value();
        func._instructions.push_back(instruction);
    }

    std::optional<ExpresserError>
    Parser::addLocalConstant(const std::string &function_name, const std::string &constant_name, std::any value) {
        auto err = getFunction(function_name);
        if (err.second.has_value())
            return err.second.value();
        auto func = err.first.value();
        if (isLocalVariable(function_name, constant_name))
            return std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
        // TODO: 堆栈上分配局部常量
    }

    std::optional<ExpresserError>
    Parser::addLocalVariable(const std::string &function_name, const std::string &variable_name, std::optional<std::any> value) {
        auto err = getFunction(function_name);
        if (err.second.has_value())
            return err.second.value();
        auto func = err.first.value();
        if (isLocalVariable(function_name, variable_name))
            return std::make_optional<ExpresserError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
        // TODO: 堆栈分配局部变量
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
        err = parseFunctions();
        if (err.has_value())
            return err;
        return {};
    }

    std::optional<ExpresserError> Parser::parseGlobalDeclarations() {
        // 0个或无数个
        for (;;) {
            // TODO: impl it
        }
        return {};
    }

    std::optional<ExpresserError> Parser::parseFunctions() {
        // 1个或无数个
        for (;;) {
            // TODO: impl it
        }
        return {};
    }
}