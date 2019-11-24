cmake_minimum_required(VERSION 3.12)
project(Expresser)

set(PROJECT_NAME expresser)
set(PROJECT_EXE ${PROJECT_NAME})

# Compiler
set(CMAKE_CXX_STANDARD 17)

# Submodule
add_subdirectory(3rd_party/argparse)

# Source Files
set(LEXER_FILES
        Lexer/Token.h
        Lexer/Lexer.h
        Lexer/Lexer.cpp)

set(PARSE_FILES
        Parser/Parser.h
        Parser/Parser.cpp)

set(SRC_FILES
        ${LEXER_FILES}
        ${PARSE_FILES}
        main.cpp)

# Target File
add_executable(${PROJECT_EXE} ${SRC_FILES})

target_link_libraries(${PROJECT_EXE} argparse)