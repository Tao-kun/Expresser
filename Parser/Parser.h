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
    const static std::map<std::string, TokenType> _function_return_type_map =
            {{"int",    TokenType::INTEGER},
             {"char",   TokenType::CHARLITERAL},
             {"double", TokenType::DOUBLE},
             {"void",   TokenType::VOID}};

    struct Constant {
        // 常量有字符串S、双浮点D、整型I三种类型
        int32_t _index{};
        char _type{};
        std::variant<int32_t, double, std::string> _value;

        Constant() = default;
        template<typename T>
        Constant(int32_t index, char type, T value);
        std::string ToCode();
    };

    struct Variable {
        int32_t _index{};
        TokenType _type{};

        Variable() = default;

        Variable(int32_t index, TokenType type) : _index(index), _type(type) {}
    };

    struct FunctionParam {
        TokenType _type;
        std::string _value;
        bool _is_const;

        FunctionParam(TokenType type, std::string value, bool is_const) :
                _type(type), _value(std::move(value)), _is_const(is_const) {}
    };

    struct Function {
        int32_t _index{};
        int32_t _name_index{};
        int32_t _params_size{};
        int32_t _level{};
        TokenType _return_type{};
        std::vector<FunctionParam> _params;

        // 局部栈顶值，初始值为参数个数
        int32_t _local_sp{};
        // 局部常量表
        std::map<std::string, Variable> _local_constants;
        // 局部变量表（已初始化）
        std::map<std::string, Variable> _local_vars;
        // 局部变量表（未初始化）
        std::map<std::string, Variable> _local_uninitialized;
        std::vector<Instruction> _instructions;
        // break和continue表
        std::vector<std::pair<int32_t, std::string>> _loop_jumps;

        // 构造函数
        Function() = default;

        Function(int32_t index, int32_t name_index, int32_t param_size, TokenType return_type,
                 std::vector<FunctionParam> params) :
                _index(index), _name_index(name_index), _params_size(param_size), _level(1),
                _return_type(return_type), _params(std::move(params)), _local_sp(param_size) {}
    };

    class Parser final {
    private:
        bool _program_end;
        uint32_t _offset;
        position_t _current_pos;
        std::vector<expresser::Token> _tokens;
        std::map<std::string, int32_t> _global_constants_index;
        // 全局常量表（栈上）
        std::map<std::string, Variable> _global_stack_constants;
        // 全局栈顶值
        int32_t _global_sp;
        // 全局变量在栈中地址
        std::map<std::string, Variable> _global_variables;
        // 全局未初始化变量在栈中地址
        std::map<std::string, Variable> _global_uninitialized;
    public:
        // 全局常量表
        std::vector<Constant> _global_constants;
        // .start段的代码
        std::vector<expresser::Instruction> _start_instruments;
        // 函数表
        std::map<std::string, Function> _functions;
    public:
        explicit Parser(std::vector<expresser::Token> _token_vector) :
                _program_end(false), _offset(0), _current_pos({0, 0}), _tokens(std::move(_token_vector)),
                _global_sp(0) {}

        std::optional<ExpresserError> Parse();
    private:
        // 辅助函数
        std::optional<Token> nextToken();
        std::optional<Token> seekToken(int32_t offset);
        void rollback();
        template<typename T>
        std::optional<ExpresserError> addGlobalConstant(const std::string &constant_name, char type, T value);
        std::optional<ExpresserError> addGlobalConstant(const std::string &variable_name, TokenType type);
        std::optional<ExpresserError> addGlobalVariable(const std::string &variable_name, TokenType type);
        std::pair<Function *, std::optional<ExpresserError>>
        addFunction(const std::string &function_name, const TokenType &return_type, const std::vector<FunctionParam> &params);
        std::optional<ExpresserError> addLocalConstant(Function &function, TokenType type, const std::string &constant_name);
        std::optional<ExpresserError> addLocalVariable(Function &function, TokenType type, const std::string &variable_name);
        std::pair<std::optional<int32_t>, std::optional<ExpresserError>> getIndex(const std::string &variable_name);
        std::pair<std::optional<int32_t>, std::optional<ExpresserError>>
        getIndex(Function &function, const std::string &variable_name);
        std::pair<Function *, std::optional<ExpresserError>> getFunction(const std::string &function_name);
        bool isGlobalVariable(const std::string &variable_name);
        bool isGlobalInitializedVariable(const std::string &variable_name);
        bool isGlobalUnInitializedVariable(const std::string &variable_name);
        bool isGlobalConstant(const std::string &variable_name);
        bool isLocalVariable(Function &function, const std::string &variable_name);
        bool isLocalInitializedVariable(Function &function, const std::string &variable_name);
        bool isLocalUnInitializedVariable(Function &function, const std::string &variable_name);
        bool isLocalConstant(Function &function, const std::string &variable_name);
        bool isInitialized(Function &function, const std::string &variable_name);
        bool isUnInitialized(Function &function, const std::string &variable_name);
        bool isConstant(Function &function, const std::string &variable_name);
        std::optional<TokenType> getVariableType(const std::string &variable_name);
        std::optional<TokenType> getVariableType(Function &function, const std::string &variable_name);
        std::optional<ExpresserError> errorFactory(ErrorCode code);

        // 语法制导翻译
        // 基础C0
        std::optional<ExpresserError> parseProgram();
        std::optional<ExpresserError> parseGlobalDeclarations();
        std::optional<ExpresserError> parseFunctionDefinitions();
        std::pair<std::optional<TokenType>, std::optional<ExpresserError>> parseExpression(Function *function);
        std::pair<std::optional<TokenType>, std::optional<ExpresserError>> parseMultiplicativeExpression(Function *function);
        std::pair<std::optional<TokenType>, std::optional<ExpresserError>> parseUnaryExpression(Function *function);
        std::pair<std::optional<TokenType>, std::optional<ExpresserError>> parsePrimaryExpression(Function *function);
        std::pair<std::optional<TokenType>, std::optional<ExpresserError>> parseFunctionCall(Function *function);
        std::optional<ExpresserError> parseParameterDeclarations(std::vector<FunctionParam> &params);
        std::optional<ExpresserError> parseFunctionDefinition();
        std::optional<ExpresserError> parseCompoundStatement(Function &function);
        std::optional<ExpresserError> parseLocalVariableDeclarations(Function &function);
        std::optional<ExpresserError> parseStatements(Function &function);
        std::pair<bool, std::optional<ExpresserError>> parseStatement(Function &function);
        std::optional<ExpresserError> parseConditionStatement(Function &function);
        std::pair<std::optional<Operation>, std::optional<ExpresserError>> parseCondition(Function &function);
        std::optional<ExpresserError> parseLoopStatement(Function &function);
        std::optional<ExpresserError> parseJumpStatement(Function &function);
        std::optional<ExpresserError> parsePrintStatement(Function &function);
        std::optional<ExpresserError> parsePrintableList(Function &function);
        std::optional<ExpresserError> parseScanStatement(Function &function);
        std::optional<ExpresserError> parseAssignmentExpression(Function &function);
        // 拓展C0
        std::pair<std::optional<TokenType>, std::optional<ExpresserError>> parseCastExpression(Function *function);

        // 静态函数
        static bool isVariableType(const Token &token);
        static bool isFunctionReturnType(const Token &token);
        static std::optional<TokenType> stringTypeToTokenType(const std::string &type_name);
    };
}
#endif //EXPRESSER_PARSER_H
