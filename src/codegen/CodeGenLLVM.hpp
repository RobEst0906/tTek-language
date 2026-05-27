#pragma once
#include "../parser/AST.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <map>
#include <string>

namespace ttek {

class CodeGenLLVM {
public:
    CodeGenLLVM(const Program& program);
    void compileToExe(const std::string& outputPath);

private:
    const Program& m_program;
    std::unique_ptr<llvm::LLVMContext> m_context;
    std::unique_ptr<llvm::Module> m_module;
    std::unique_ptr<llvm::IRBuilder<>> m_builder;
    std::map<std::string, llvm::AllocaInst*> m_namedValues;
    std::map<std::string, llvm::Function*> m_functions;

    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func, const std::string& name, llvm::Type* type);
    llvm::Type* getLLVMType(const Type& t);
    llvm::Value* genExpr(const Expr& expr);
    void genStmt(const Stmt& stmt);
    void genBlock(const std::vector<StmtPtr>& block);
    void genGlobalVar(const GlobalVar& var);
    void genRule(const Rule& rule);
    void genFunction(const Function& func);
    void genInitBlock();
    llvm::Value* getNamedValue(const std::string& name);
};

} // namespace ttek