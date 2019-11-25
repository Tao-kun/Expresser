#include <cstdio>
#include <fstream>
#include <iostream>
#include "argparse.hpp"

void assembly(std::istream &_input, std::ostream &_output) {

}

void binary(std::istream &_input, std::ostream &_output) {

}

int main(int argc, char **argv) {
    argparse::ArgumentParser arg("expresser");
    arg.add_argument("input")
            .required()
            .help("Source code file");
    arg.add_argument("-s")
            .default_value(false)
            .implicit_value(true)
            .help("Assembly");
    arg.add_argument("-c")
            .default_value(false)
            .implicit_value(true)
            .help("Binary");
    arg.add_argument("-o", "--output")
            .help("Output file");
    try {
        arg.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err) {
        ::fprintf(stderr, "Argument Error %s\n\n", err.what());
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
        ::fprintf(stderr, "No input file\n\n");
        exit(3);
    } else {
        infs.open(input_file, std::ios::in);
        if (!infs) {
            ::fprintf(stderr, "Open file %s error\n\n", input_file.c_str());
            exit(3);
        }
        input = &infs;
    }

    if (output_file.empty()) {
        output_file = "out";
    }
    outfs.open(output_file, std::ios::out | std::ios::trunc);
    if (!outfs) {
        ::fprintf(stderr, "Create file %s error\n\n", output_file.c_str());
        exit(3);
    }
    output = &outfs;

    if (arg["-s"] == true && arg["-c"] == true) {
        ::fprintf(stderr, "Cannot run lexer and parser at once\n");
        exit(2);
    }
    if (arg["-s"] == true) {
        assembly(*input, *output);
    } else if (arg["-c"] == true) {
        binary(*input, *output);
    } else {
        ::fprintf(stderr, "Must choose running lexer or parser\n");
    }
    return 0;
}