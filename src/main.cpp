#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "codegen/CodeGenLLVM.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ttek_compiler <source.ttek>\n";
        return 1;
    }

    std::cerr << "[1] Opening file...\n"; std::cerr.flush();
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Could not open file: " << argv[1] << '\n';
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    std::cerr << "[2] Lexing...\n"; std::cerr.flush();

    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    std::cerr << "[3] Parsing... (" << tokens.size() << " tokens)\n"; std::cerr.flush();

    ttek::Parser parser(tokens);
    ttek::Program program = parser.parse();
    std::cerr << "[4] Parsed OK. globals=" << program.globals.size()
              << " rules=" << program.rules.size()
              << " initBlock=" << program.initBlock.size() << "\n"; std::cerr.flush();

    std::string inFile(argv[1]);
    std::string objFile = inFile.substr(0, inFile.find_last_of('.')) + ".o";

    std::cerr << "[5] Creating CodeGen...\n"; std::cerr.flush();
    ttek::CodeGenLLVM codegen(program);
    std::cerr << "[6] Compiling to exe: " << objFile << "\n"; std::cerr.flush();
    codegen.compileToExe(objFile);
    std::cerr << "[7] Done!\n"; std::cerr.flush();

    std::cout << "Compiled " << inFile << " -> " << inFile.substr(0, inFile.find_last_of('.')) << ".exe\n";
    return 0;
}