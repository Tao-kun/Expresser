cmake_minimum_required(VERSION 3.12)
project(Expresser)

set(PROJECT_NAME expresser)
set(PROJECT_EXE ${PROJECT_NAME})
set(PROJECT_LIB ${PROJECT_EXE}_lib)

# Compiler
set(CMAKE_CXX_STANDARD 17)

# Submodule
add_subdirectory(3rd_party/argparse)

# Source Files
set(LIB_FILES
        Lexer/Token.h
        Lexer/Lexer.h
        Lexer/Lexer.cpp
        Util/Error.h
        Parser/Parser.h
        Parser/Parser.cpp)

set(MAIN_FILES
        ${LIB_FILES}
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

target_link_libraries(${PROJECT_EXE} ${PROJECT_LIB} argparse)