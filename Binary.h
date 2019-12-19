#ifndef EXPRESSER_BINARY_HPP
#define EXPRESSER_BINARY_HPP

#include <vector>

#include "Lexer/Lexer.h"
#include "Parser/Parser.h"
#include "Instruction/Instruction.h"

const static std::vector<uint8_t> magic = {0x43, 0x30, 0x3a, 0x29};
const static std::vector<uint8_t> version = {0x00, 0x00, 0x00, 0x01};

void write_binary_to_file(const expresser::Parser &_parser, std::ostream &_output) {
    uint8_t *cpy_pointer;
    std::vector<uint8_t> result;

    result.insert(result.end(), magic.begin(), magic.end());
    result.insert(result.end(), version.begin(), version.end());

    // constant
    auto constants_count = (uint16_t) _parser._global_constants.size();
    cpy_pointer = (uint8_t *) &constants_count;
    for (int i = 1; i >= 0; i--)
        result.push_back(cpy_pointer[i]);
    for (auto constant:_parser._global_constants) {
        auto constant_binary = constant.ToBinary();
        result.insert(result.end(), constant_binary.begin(), constant_binary.end());
    }

    // start code
    auto start_code_count = (uint16_t) _parser._start_instruments.size();
    cpy_pointer = (uint8_t *) &start_code_count;
    for (int i = 1; i >= 0; i--)
        result.push_back(cpy_pointer[i]);
    for (auto instrument:_parser._start_instruments) {
        auto instrument_bin = instrument.ToBinary();
        result.insert(result.end(), instrument_bin.begin(), instrument_bin.end());
    }

    // functions
    std::vector<std::pair<int32_t, std::string>> function_information;
    function_information.reserve(_parser._functions.size());
    for (const auto &function:_parser._functions) {
        function_information.emplace_back(std::make_pair(function.second._index, function.first));
    }
    std::sort(function_information.begin(), function_information.end());
    auto function_count = function_information.size();
    cpy_pointer = (uint8_t *) &function_count;
    for (int i = 1; i >= 0; i--)
        result.push_back(cpy_pointer[i]);
    for (const auto &it:function_information) {
        auto function = _parser._functions.find(it.second)->second;
        auto func_bin = function.ToBinary();
        result.insert(result.end(), func_bin.begin(), func_bin.end());
    }

    for (uint8_t ch:result)
        _output.put(ch);
}

#endif //EXPRESSER_BINARY_HPP