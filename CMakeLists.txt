cmake_minimum_required(VERSION 3.12)
project(Expresser)

set(PROJECT_NAME expresser)
set(PROJECT_EXE ${PROJECT_NAME})
set(PROJECT_LIB ${PROJECT_EXE}_lib)

# Compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")

# Submodule
add_subdirectory(3rd_party/argparse)
add_subdirectory(3rd_party/fmt)

# Source Files
set(LIB_FILES
        Types.h
        Error/Error.h
        Instruction/Instruction.h
        Lexer/Token.h
        Lexer/Lexer.h
        Lexer/Lexer.cpp
        Lexer/Utils.hpp
        Parser/Parser.h
        Parser/Parser.cpp)

set(MAIN_FILES
        fmts.hpp
        main.cpp)

# Target File
add_library(${PROJECT_LIB} ${LIB_FILES})

add_executable(${PROJECT_EXE} ${MAIN_FILES})

set_target_properties(${PROJECT_EXE} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_LIB} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON)
target_include_directories(${PROJECT_EXE} PRIVATE .)
target_include_directories(${PROJECT_LIB} PRIVATE .)

target_compile_options(${PROJECT_EXE} PRIVATE -Wall -g -O2 -ftemplate-backtrace-limit=0)
target_compile_options(${PROJECT_LIB} PRIVATE -Wall -g -O2 -ftemplate-backtrace-limit=0)

target_link_libraries(${PROJECT_EXE} ${PROJECT_LIB} argparse fmt::fmt)