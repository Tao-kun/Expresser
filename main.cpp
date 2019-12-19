#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include "argparse.hpp"
#include "fmt/core.h"

#include "Binary.h"
#include "fmts.hpp"
#include "Lexer/Lexer.h"
#include "Parser/Parser.h"

std::vector<expresser::Token> _allToken(std::istream &_input) {
    expresser::Lexer lex(_input);
    auto res = lex.AllTokens();
    if (res.second.has_value()) {
        fmt::print(stderr, "Lexer error: {}\n", res.second.value());
        exit(2);
    }
    return res.first;
}

void write_assembly_to_file(const expresser::Parser &_parser, std::ostream &_output) {
    _output << ".constants:\n";
    for (auto constant:_parser._global_constants) {
        _output << fmt::format("{}\n", constant.ToCode());
    }
    _output << ".start:\n";
    for (expresser::Instruction instrument:_parser._start_instruments) {
        _output << fmt::format("{}\n", instrument);
    }
    auto functions = _parser._functions;
    std::vector<std::pair<int32_t, std::string>> function_information;
    function_information.reserve(functions.size());
    for (const auto &function:functions) {
        function_information.emplace_back(std::make_pair(function.second._index, function.first));
    }
    std::sort(function_information.begin(), function_information.end());
    _output << ".functions:\n";
    for (const auto &it:function_information) {
        auto function = functions.find(it.second);
        _output << fmt::format("{} {} {} {}\n", function->second._index, function->second._name_index,
                               function->second._params_size, function->second._level);
    }
    for (const auto &it:function_information) {
        auto function = functions.find(it.second);
        _output << fmt::format(".F{}:\n", it.first);
        for (const auto &instrument:function->second._instructions) {
            _output << fmt::format("{}\n", instrument);
        }
    }
}


void assembly(std::istream &_input, std::ostream &_output) {
    auto tks = _allToken(_input);
    expresser::Parser parser(tks);
    auto err = parser.Parse();
    if (err.has_value()) {
        fmt::print(stderr, "Parser error: {}\n", err.value());
        exit(2);
    }
    write_assembly_to_file(parser, _output);
}

void binary(std::istream &_input, std::ostream &_output) {
    auto tks = _allToken(_input);
    expresser::Parser parser(tks);
    auto err = parser.Parse();
    if (err.has_value()) {
        fmt::print(stderr, "Parser error: {}\n", err.value());
        exit(2);
    }
    write_binary_to_file(parser, _output);
}

int main(int argc, char **argv) {
    argparse::ArgumentParser arg("c0");
    arg.add_argument("input")
            .required()
            .help("Source code file");
    arg.add_argument("-c")
            .default_value(false)
            .implicit_value(true)
            .help("Binary");
    arg.add_argument("-s")
            .default_value(false)
            .implicit_value(true)
            .help("Assembly");
    arg.add_argument("-o", "--output")
            .default_value(std::string(""))
            .help("Output file");
    try {
        arg.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err) {
        std::cerr << "Argument Error " << err.what() << std::endl;
        std::cout << arg;
        exit(2);
    }

    auto input_file = arg.get<std::string>("input");
    auto output_file = arg.get<std::string>("--output");
    std::istream *input;
    std::ostream *output;
    std::ifstream infs;
    std::ofstream outfs;
    if (input_file.empty()) {
        std::cerr << "No input file" << std::endl;
        exit(3);
    } else {
        infs.open(input_file, std::ios::in);
        if (!infs) {
            std::cerr << "Open file " << input_file << " error" << std::endl;
            exit(3);
        }
        input = &infs;
    }

    if (output_file.empty()) {
        output_file = "out";
    }
    if (arg["-s"] == true)
        outfs.open(output_file, std::ios::out | std::ios::trunc);
    else
        outfs.open(output_file, std::ios::out | std::ios::binary);
    if (!outfs) {
        std::cerr << "Create file " << output_file << " error" << std::endl;
        exit(3);
    }
    output = &outfs;

    if (arg["-c"] == true && arg["-s"] == true) {
        std::cerr << "Cannot run lexer and parser at once" << std::endl;
        exit(2);
    }
    if (arg["-s"] == true)
        assembly(*input, *output);
    else if (arg["-c"] == true)
        binary(*input, *output);
    else
        std::cerr << "Must choose running lexer or parser" << std::endl;
    return 0;
}