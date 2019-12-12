#ifndef EXPRESSER_PARSER_H
#define EXPRESSER_PARSER_H

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "Lexer/Token.h"
#include "Instruction/Instruction.h"

namespace expresser {
    // 静态常量
    const static std::set<TokenType> _variable_type_set = {INTEGER, DOUBLE, CHARLITERAL};
    const static std::map<std::string, TokenType> _variable_type_map =
            {{"int",    TokenType::INTEGER},
             {"char",   TokenType::CHARLITERAL},
             {"double", TokenType::DOUBLE}};

    struct Constant {
        // 常量有字符串S、双浮点D、整型I三种类型
        int _index{};
        char _type{};
        std::variant<int32_t, double, std::string> _value;

        Constant() = default;
        template<typename T>
        Constant(int index, char type, T value);
        std::string ToCode();
    };

    struct FunctionParam {
        std::string _type;
        std::string _value;
    };

    struct Function {
        int32_t _index;
        int32_t _name_index;
        int32_t _params_size;
        int32_t _level;
        std::vector<FunctionParam> _params;

        // 局部栈顶值
        int32_t _local_sp;
        // 局部常量表
        std::map<std::string, int32_t> _local_constants;
        // 局部变量表（已初始化）
        std::map<std::string, int32_t> _local_vars;
        // 局部变量表（未初始化）
        std::map<std::string, int32_t> _local_uninitialized;
        std::vector<Instruction> _instructions;

        // 构造函数
        Function(int32_t index, int32_t name_index, int32_t param_size, std::vector<FunctionParam> params) :
                _index(index), _name_index(name_index), _params_size(param_size), _level(1), _params(std::move(params)), _local_sp(0) {}
    };

    class Parser final {
    private:
        uint32_t _offset;
        position_t _current_pos;
        std::vector<expresser::Token> _tokens;
        // .start段的代码
        std::vector<expresser::Instruction> _start_instruments;
        // 全局常量表
        std::map<std::string, Constant> _global_constants;
        // 全局栈顶值
        int32_t _global_sp;
        // 全局变量在栈中地址
        std::map<std::string, int32_t> _global_variables;
        // 全局未初始化变量在栈中地址
        std::map<std::string, int32_t> _global_uninitialized;
        // 函数表
        std::map<std::string, Function> _functions;
    public:
        explicit Parser(std::vector<expresser::Token> _token_vector) :
                _offset(0), _current_pos({0, 0}), _tokens(std::move(_token_vector)), _global_sp(0) {}

        std::optional<ExpresserError> Parse();
        void WriteToFile(std::ostream &_output);
    private:
        // 辅助函数
        std::optional<Token> nextToken();
        template<typename T>
        std::optional<ExpresserError> addGlobalConstant(const std::string &constant_name, char type, T value);
        std::optional<ExpresserError> addGlobalVariable(const std::string &variable_name, std::optional<std::any> value);
        std::optional<ExpresserError> addFunction(const std::string &function_name, const std::vector<FunctionParam>& params);
        std::optional<ExpresserError> addFunctionInstrument(const std::string& function_name, Instruction instruction);
        std::optional<ExpresserError> addLocalConstant(const std::string &function_name, const std::string &constant_name, std::any value);
        std::optional<ExpresserError>
        addLocalVariable(const std::string &function_name, const std::string &variable_name, std::optional<std::any> value);
        std::pair<std::optional<int32_t>, std::optional<ExpresserError>>
        getIndex(const std::string &function_name, const std::string &variable_name);
        std::pair<std::optional<Function>, std::optional<ExpresserError>> getFunction(const std::string &function_name);
        bool isGlobalVariable(const std::string &variable_name);
        bool isGlobalInitializedVariable(const std::string &variable_name);
        bool isGlobalUnInitializedVariable(const std::string &variable_name);
        bool isGlobalConstant(const std::string &variable_name);
        bool isLocalVariable(const std::string &function_name, const std::string &variable_name);
        bool isLocalInitializedVariable(const std::string &function_name, const std::string &variable_name);
        bool isLocalUnInitializedVariable(const std::string &function_name, const std::string &variable_name);
        bool isLocalConstant(const std::string &function_name, const std::string &variable_name);
        bool isInitialized(const std::string &function_name, const std::string &variable_name);
        bool isUnInitialized(const std::string &function_name, const std::string &variable_name);
        bool isConstant(const std::string &function_name, const std::string &variable_name);

        // 语法制导翻译
        // 基础C0
        std::optional<ExpresserError> parseProgram();
        std::optional<ExpresserError> parseGlobalDeclarations();
        std::optional<ExpresserError> parseFunctionDefinitions();
        std::optional<ExpresserError> parseExpression(TokenType type);
        std::optional<ExpresserError> parseMultiplicativeExpression(TokenType type);
        std::optional<ExpresserError> parseUnaryExpression(TokenType type);
        std::optional<ExpresserError> parsePrimaryExpression(TokenType type);
        std::optional<ExpresserError> parseFunctionCall();
        std::optional<ExpresserError> parseParameterDeclarations();
        std::optional<ExpresserError> parseFunctionDefinition();
        std::optional<ExpresserError> parseCompoundStatement();
        std::optional<ExpresserError> parseLocalVariableDeclarations();
        std::optional<ExpresserError> parseStatements();
        std::optional<ExpresserError> parseConditionStatement();
        std::optional<ExpresserError> parseLoopStatement();
        std::optional<ExpresserError> parseJumpStatement();
        std::optional<ExpresserError> parsePrintStatement();
        std::optional<ExpresserError> parseScanStatement();
        std::optional<ExpresserError> parseAssignmentExpression();
        // 拓展C0
        std::optional<ExpresserError> parseCastExpression();

        // 静态函数
        static bool isVariableType(const Token &token);
        static std::optional<TokenType> stringTypeToTokenType(const std::string &type_name);
    };
}
#endif //EXPRESSER_PARSER_H
