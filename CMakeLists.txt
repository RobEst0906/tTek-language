cmake_minimum_required(VERSION 3.16)
project(ttek_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

execute_process(COMMAND llvm-config --libs core executionengine mcjit nativecodegen
                OUTPUT_VARIABLE LLVM_LINK_LIBS
                OUTPUT_STRIP_TRAILING_WHITESPACE)

add_executable(ttek_compiler
    src/main.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
    src/codegen/CodeGenLLVM.cpp
)

target_include_directories(ttek_compiler PRIVATE src)
target_link_libraries(ttek_compiler ${LLVM_LINK_LIBS})

if(WIN32)
    target_link_libraries(ttek_compiler kernel32)
endif()