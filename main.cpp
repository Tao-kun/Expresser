#include <fstream>
#include <iostream>
#include "argparse.hpp"
#include "Lexer/Lexer.h"

void lexer(std::istream &_input, std::ostream &_output) {
    expresser::Lexer lex(_input);
    auto res = lex.AllTokens();
    if (res.second.has_value()) {
        std::cerr << "Lexer error: " << res.second->ErrorInfo() << std::endl;
        exit(2);
    }
    for (auto token:res.first) {
        std::cout << token.GetInfo() << std::endl;
    }
}

void assembly(std::istream &_input, std::ostream &_output) {

}

void binary(std::istream &_input, std::ostream &_output) {

}

int main(int argc, char **argv) {
    argparse::ArgumentParser arg("expresser");
    arg.add_argument("input")
            .required()
            .help("Source code file");
    arg.add_argument("-l")
            .default_value(false)
            .implicit_value(true)
            .help("Lexer");
    arg.add_argument("-s")
            .default_value(false)
            .implicit_value(true)
            .help("Assembly");
    arg.add_argument("-c")
            .default_value(false)
            .implicit_value(true)
            .help("Binary");
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
    outfs.open(output_file, std::ios::out | std::ios::trunc);
    if (!outfs) {
        std::cerr << "Create file " << output_file << " error" << std::endl;
        exit(3);
    }
    output = &outfs;

    if (arg["-l"] == true && arg["-s"] == true) {
        std::cerr << "Cannot run lexer and parser at once" << std::endl;
        exit(2);
    }
    if (arg["-s"] == true) {
        assembly(*input, *output);
    } else if (arg["-l"] == true) {
        lexer(*input, *output);
    } else {
        if (arg["-c"] == true)
            binary(*input, *output);
        else
            std::cerr << "Must choose running lexer or parser" << std::endl;
    }
    return 0;
}