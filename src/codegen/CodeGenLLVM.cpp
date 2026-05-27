#include "CodeGenLLVM.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/IR/LegacyPassManager.h>
#include <stdexcept>
#include <algorithm>
#include <cstdlib>

namespace ttek {

CodeGenLLVM::CodeGenLLVM(const Program& program)
    : m_program(program),
      m_context(std::make_unique<llvm::LLVMContext>()),
      m_module(std::make_unique<llvm::Module>("ttek", *m_context)),
      m_builder(std::make_unique<llvm::IRBuilder<>>(*m_context)) {

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
}

void CodeGenLLVM::compileToExe(const std::string& outputPath) {
    for (const auto& var : m_program.globals) {
        genGlobalVar(var);
    }
    for (const auto& func : m_program.functions) {
        genFunction(func);
    }

    
    llvm::Function* rulesFunc = nullptr;
    if (!m_program.rules.empty()) {
        std::vector<const Rule*> sortedRules;
        for (const auto& r : m_program.rules) sortedRules.push_back(&r);
        std::sort(sortedRules.begin(), sortedRules.end(),
            [](const Rule* a, const Rule* b) { return a->priority < b->priority; });

        llvm::FunctionType* rulesType = llvm::FunctionType::get(llvm::Type::getVoidTy(*m_context), false);
        rulesFunc = llvm::Function::Create(rulesType, llvm::Function::InternalLinkage, "ttek_rules", m_module.get());
        llvm::BasicBlock* rulesEntry = llvm::BasicBlock::Create(*m_context, "entry", rulesFunc);
        m_builder->SetInsertPoint(rulesEntry);

        llvm::BasicBlock* loopCond = llvm::BasicBlock::Create(*m_context, "loop.cond", rulesFunc);
        m_builder->CreateBr(loopCond);
        m_builder->SetInsertPoint(loopCond);

        llvm::BasicBlock* loopBody = llvm::BasicBlock::Create(*m_context, "loop.body", rulesFunc);
        llvm::BasicBlock* loopEnd = llvm::BasicBlock::Create(*m_context, "loop.end", rulesFunc);
        m_builder->CreateCondBr(llvm::ConstantInt::getTrue(*m_context), loopBody, loopEnd);
        m_builder->SetInsertPoint(loopBody);

        for (const auto* rule : sortedRules) {
            llvm::Value* cond = genExpr(*rule->condition);
            // i1 -> i1 (already bool from ICmp), but make sure it's i1
            if (cond->getType()->getIntegerBitWidth() != 1) {
                cond = m_builder->CreateICmpNE(cond,
                    llvm::ConstantInt::get(cond->getType(), 0), "tobool");
            }
            llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(*m_context, "rule.then", rulesFunc);
            llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(*m_context, "rule.else", rulesFunc);
            m_builder->CreateCondBr(cond, thenBlock, elseBlock);
            m_builder->SetInsertPoint(thenBlock);
            genBlock(rule->body);
            
            if (!m_builder->GetInsertBlock()->getTerminator()) {
                m_builder->CreateBr(elseBlock);
            }
            m_builder->SetInsertPoint(elseBlock);
        }
        if (!m_builder->GetInsertBlock()->getTerminator()) {
            m_builder->CreateBr(loopCond);
        }
        m_builder->SetInsertPoint(loopEnd);
        m_builder->CreateRetVoid();
    }

    // Build main function
    llvm::FunctionType* mainType = llvm::FunctionType::get(llvm::Type::getInt32Ty(*m_context), false);
    llvm::Function* mainFunc = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", m_module.get());
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*m_context, "entry", mainFunc);
    m_builder->SetInsertPoint(entry);

    genInitBlock();

    if (rulesFunc) {
        m_builder->CreateCall(rulesFunc);
    }

    m_builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*m_context), 0));

    llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
    m_module->setTargetTriple(triple);

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(triple.str(), error);
    if (!target) throw std::runtime_error(error);

    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(triple, "generic", "", opt, llvm::Reloc::Static, llvm::CodeModel::Large);

    m_module->setDataLayout(targetMachine->createDataLayout());

    std::error_code ec;
    llvm::raw_fd_ostream dest(outputPath, ec, llvm::sys::fs::OF_None);
    if (ec) throw std::runtime_error("Could not open file: " + outputPath);

    llvm::legacy::PassManager pm;
    if (targetMachine->addPassesToEmitFile(pm, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        throw std::runtime_error("Target cannot emit object file");
    }
    pm.run(*m_module);
    dest.close();

    std::string linkCmd = "gcc " + outputPath + " -o " + outputPath.substr(0, outputPath.size()-2) + ".exe -mconsole -lkernel32";
    system(linkCmd.c_str());
}

llvm::Type* CodeGenLLVM::getLLVMType(const Type& t) {
    switch (t.kind) {
        case TypeKind::U8: case TypeKind::I8: return llvm::Type::getInt8Ty(*m_context);
        case TypeKind::U16: case TypeKind::I16: return llvm::Type::getInt16Ty(*m_context);
        case TypeKind::U32: case TypeKind::I32: return llvm::Type::getInt32Ty(*m_context);
        case TypeKind::U64: case TypeKind::I64: return llvm::Type::getInt64Ty(*m_context);
        case TypeKind::F32: return llvm::Type::getFloatTy(*m_context);
        case TypeKind::F64: return llvm::Type::getDoubleTy(*m_context);
        case TypeKind::PTR: return llvm::PointerType::get(*m_context, 0);
        case TypeKind::ARRAY: return llvm::ArrayType::get(getLLVMType(*t.base), t.arraySize);
        case TypeKind::VOID: return llvm::Type::getVoidTy(*m_context);
    }
    return llvm::Type::getVoidTy(*m_context);
}

llvm::AllocaInst* CodeGenLLVM::createEntryBlockAlloca(llvm::Function* func, const std::string& name, llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

llvm::Value* CodeGenLLVM::genExpr(const Expr& expr) {
    if (auto* e = dynamic_cast<const IntLiteral*>(&expr)) {
        return llvm::ConstantInt::get(getLLVMType(Type::makeSimple(TypeKind::I32)), e->value, true);
    }
    if (auto* e = dynamic_cast<const FloatLiteral*>(&expr)) {
        return llvm::ConstantFP::get(*m_context, llvm::APFloat(e->value));
    }
    if (auto* e = dynamic_cast<const StringLiteral*>(&expr)) {
        return m_builder->CreateGlobalString(e->value);
    }
    if (auto* e = dynamic_cast<const Identifier*>(&expr)) {
        llvm::Value* val = getNamedValue(e->name);
        if (!val) throw std::runtime_error("Undefined variable: " + e->name);
        return m_builder->CreateLoad(getLLVMType(Type::makeSimple(TypeKind::I32)), val, e->name.c_str());
    }
    if (auto* e = dynamic_cast<const BinaryOp*>(&expr)) {
        if (e->op == "=") {
            llvm::Value* lhs = getNamedValue(dynamic_cast<Identifier*>(e->left.get())->name);
            llvm::Value* rhs = genExpr(*e->right);
            m_builder->CreateStore(rhs, lhs);
            return rhs;
        }
        llvm::Value* l = genExpr(*e->left);
        llvm::Value* r = genExpr(*e->right);
        if (e->op == "+") return m_builder->CreateAdd(l, r, "addtmp");
        if (e->op == "-") return m_builder->CreateSub(l, r, "subtmp");
        if (e->op == "*") return m_builder->CreateMul(l, r, "multmp");
        if (e->op == "/") return m_builder->CreateSDiv(l, r, "divtmp");
        if (e->op == "%") return m_builder->CreateSRem(l, r, "remtmp");
        if (e->op == "==") return m_builder->CreateICmpEQ(l, r, "eqtmp");
        if (e->op == "!=") return m_builder->CreateICmpNE(l, r, "netmp");
        if (e->op == "<") return m_builder->CreateICmpSLT(l, r, "lttmp");
        if (e->op == ">") return m_builder->CreateICmpSGT(l, r, "gttmp");
        if (e->op == "<=") return m_builder->CreateICmpSLE(l, r, "letmp");
        if (e->op == ">=") return m_builder->CreateICmpSGE(l, r, "getmp");
        if (e->op == "&&") return m_builder->CreateAnd(l, r, "andtmp");
        if (e->op == "||") return m_builder->CreateOr(l, r, "ortmp");
        throw std::runtime_error("Unknown binary operator: " + e->op);
    }
    if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        std::string funcName = e->callee;
        if (funcName == "print") funcName = "printf";
        if (funcName == "sleep") {
            
            llvm::Function* sleepFn = m_module->getFunction("Sleep");
            if (!sleepFn) {
                llvm::FunctionType* ft = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(*m_context),
                    {llvm::Type::getInt32Ty(*m_context)}, false);
                sleepFn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "Sleep", m_module.get());
            }
            std::vector<llvm::Value*> args;
            for (auto& arg : e->args) args.push_back(genExpr(*arg));
            return m_builder->CreateCall(sleepFn, args);
        }
        llvm::Function* callee = m_module->getFunction(funcName);
        if (!callee) {
            std::vector<llvm::Type*> argTypes;
            for (size_t i = 0; i < e->args.size(); i++)
                argTypes.push_back(llvm::PointerType::getUnqual(*m_context));
            llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getInt32Ty(*m_context), argTypes, true);
            callee = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, funcName, m_module.get());
        }
        std::vector<llvm::Value*> args;
        for (auto& arg : e->args) args.push_back(genExpr(*arg));
        return m_builder->CreateCall(callee, args);
    }
    throw std::runtime_error("Unknown expression");
}

void CodeGenLLVM::genStmt(const Stmt& stmt) {
    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        llvm::Function* func = m_builder->GetInsertBlock()->getParent();
        llvm::Type* type = getLLVMType(s->type);
        llvm::AllocaInst* alloca = createEntryBlockAlloca(func, s->name, type);
        m_namedValues[s->name] = alloca;
        if (s->initializer) {
            llvm::Value* initVal = genExpr(*s->initializer);
            m_builder->CreateStore(initVal, alloca);
        }
    } else if (auto* s = dynamic_cast<const AssignStmt*>(&stmt)) {
        llvm::Value* val = genExpr(*s->rhs);
        if (auto* id = dynamic_cast<const Identifier*>(s->lhs.get())) {
            llvm::Value* ptr = getNamedValue(id->name);
            m_builder->CreateStore(val, ptr);
        } else {
            throw std::runtime_error("Invalid assignment target");
        }
    } else if (auto* s = dynamic_cast<const CallStmt*>(&stmt)) {
        std::vector<llvm::Value*> args;
        for (auto& arg : s->args) args.push_back(genExpr(*arg));
        std::string funcName = s->name;
        if (funcName == "print") funcName = "printf";
        if (funcName == "sleep") {
            llvm::Function* sleepFn = m_module->getFunction("Sleep");
            if (!sleepFn) {
                llvm::FunctionType* ft = llvm::FunctionType::get(
                    llvm::Type::getVoidTy(*m_context),
                    {llvm::Type::getInt32Ty(*m_context)}, false);
                sleepFn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "Sleep", m_module.get());
            }
            m_builder->CreateCall(sleepFn, args);
        } else {
            llvm::Function* f = m_module->getFunction(funcName);
            if (!f) {
                std::vector<llvm::Type*> argTypes;
                for (size_t i = 0; i < args.size(); i++)
                    argTypes.push_back(llvm::PointerType::getUnqual(*m_context));
                llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getInt32Ty(*m_context), argTypes, true);
                f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, funcName, m_module.get());
            }
            m_builder->CreateCall(f, args);
        }
    } else if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        llvm::Value* cond = genExpr(*s->cond);
        llvm::Function* func = m_builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*m_context, "then", func);
        llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*m_context, "else");
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*m_context, "ifcont");
        m_builder->CreateCondBr(cond, thenBB, elseBB);

        m_builder->SetInsertPoint(thenBB);
        genBlock(s->thenBody);
        m_builder->CreateBr(mergeBB);
        thenBB = m_builder->GetInsertBlock();

        func->insert(func->end(), elseBB);
        m_builder->SetInsertPoint(elseBB);
        genBlock(s->elseBody);
        m_builder->CreateBr(mergeBB);
        elseBB = m_builder->GetInsertBlock();

        func->insert(func->end(), mergeBB);
        m_builder->SetInsertPoint(mergeBB);
    } else if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        llvm::Function* func = m_builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*m_context, "while.cond", func);
        llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(*m_context, "while.body");
        llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*m_context, "while.end");
        m_builder->CreateBr(condBB);
        m_builder->SetInsertPoint(condBB);
        llvm::Value* cond = genExpr(*s->cond);
        m_builder->CreateCondBr(cond, bodyBB, endBB);
        func->insert(func->end(), bodyBB);
        m_builder->SetInsertPoint(bodyBB);
        genBlock(s->body);
        m_builder->CreateBr(condBB);
        func->insert(func->end(), endBB);
        m_builder->SetInsertPoint(endBB);
    } else if (auto* s = dynamic_cast<const RetStmt*>(&stmt)) {
        if (s->value) {
            m_builder->CreateRet(genExpr(*s->value));
        } else {
            m_builder->CreateRetVoid();
        }
    } else if (dynamic_cast<const HaltStmt*>(&stmt)) {
        llvm::FunctionType* exitType = llvm::FunctionType::get(llvm::Type::getVoidTy(*m_context), {llvm::Type::getInt32Ty(*m_context)}, false);
        llvm::Function* exitFunc = m_module->getFunction("exit");
        if (!exitFunc) exitFunc = llvm::Function::Create(exitType, llvm::Function::ExternalLinkage, "exit", m_module.get());
        m_builder->CreateCall(exitFunc, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*m_context), 0)});
        m_builder->CreateUnreachable();
    } else if (auto* s = dynamic_cast<const ExprStmt*>(&stmt)) {
        genExpr(*s->expr);
    }
}

void CodeGenLLVM::genBlock(const std::vector<StmtPtr>& block) {
    for (const auto& stmt : block) {
        genStmt(*stmt);
    }
}

void CodeGenLLVM::genGlobalVar(const GlobalVar& var) {
    llvm::Type* type = getLLVMType(var.type);
    m_module->getOrInsertGlobal(var.name, type);
    llvm::GlobalVariable* gv = m_module->getNamedGlobal(var.name);
    gv->setLinkage(llvm::GlobalValue::InternalLinkage);
    if (var.initializer) {
        if (auto* lit = dynamic_cast<const IntLiteral*>(var.initializer.get())) {
            gv->setInitializer(llvm::ConstantInt::get(*m_context, llvm::APInt(32, lit->value, true)));
        } else {
            gv->setInitializer(llvm::ConstantInt::get(*m_context, llvm::APInt(32, 0)));
        }
    } else {
        gv->setInitializer(llvm::Constant::getNullValue(type));
    }
}

llvm::Value* CodeGenLLVM::getNamedValue(const std::string& name) {
    if (m_namedValues.find(name) != m_namedValues.end()) {
        return m_namedValues[name];
    }
    llvm::GlobalVariable* gv = m_module->getNamedGlobal(name);
    if (gv) return gv;
    throw std::runtime_error("Unknown variable: " + name);
}

void CodeGenLLVM::genFunction(const Function& func) {
    std::vector<llvm::Type*> paramTypes;
    for (auto& p : func.params) paramTypes.push_back(getLLVMType(p.second));
    llvm::FunctionType* ft = llvm::FunctionType::get(getLLVMType(func.returnType), paramTypes, false);
    llvm::Function* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, func.name, m_module.get());

    size_t idx = 0;
    for (auto& arg : f->args()) {
        arg.setName(func.params[idx].first);
        idx++;
    }

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*m_context, "entry", f);
    m_builder->SetInsertPoint(entry);
    m_namedValues.clear();
    for (auto& arg : f->args()) {
        llvm::AllocaInst* alloca = createEntryBlockAlloca(f, std::string(arg.getName()), arg.getType());
        m_builder->CreateStore(&arg, alloca);
        m_namedValues[std::string(arg.getName())] = alloca;
    }

    genBlock(func.body);
    if (func.returnType.kind == TypeKind::VOID) m_builder->CreateRetVoid();
    llvm::verifyFunction(*f);
}

void CodeGenLLVM::genInitBlock() {
    if (!m_program.initBlock.empty()) {
        genBlock(m_program.initBlock);
    }
}

} // namespace ttek
