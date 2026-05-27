#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace ttek {

enum class StorageClass { MEM, REG };

enum class TypeKind { U8, U16, U32, U64, I8, I16, I32, I64, F32, F64, PTR, ARRAY, VOID };

struct Type {
    TypeKind kind;
    std::unique_ptr<Type> base;
    int arraySize = 0;

    Type() : kind(TypeKind::VOID) {}
    Type(TypeKind k) : kind(k) {}
    Type(const Type& other) : kind(other.kind), arraySize(other.arraySize) {
        if (other.base) base = std::make_unique<Type>(*other.base);
    }
    Type& operator=(const Type& other) {
        if (this != &other) {
            kind = other.kind;
            arraySize = other.arraySize;
            base = other.base ? std::make_unique<Type>(*other.base) : nullptr;
        }
        return *this;
    }
    Type(Type&&) = default;
    Type& operator=(Type&&) = default;

    static Type makeSimple(TypeKind k) { return Type(k); }
    static Type makePtr(std::unique_ptr<Type> base) { Type t(TypeKind::PTR); t.base = std::move(base); return t; }
    static Type makeArray(std::unique_ptr<Type> base, int size) { Type t(TypeKind::ARRAY); t.base = std::move(base); t.arraySize = size; return t; }
};

struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct Expr { virtual ~Expr() = default; };

struct IntLiteral : Expr { int64_t value; explicit IntLiteral(int64_t v) : value(v) {} };
struct FloatLiteral : Expr { double value; explicit FloatLiteral(double v) : value(v) {} };
struct StringLiteral : Expr { std::string value; explicit StringLiteral(std::string v) : value(std::move(v)) {} };
struct CharLiteral : Expr { char value; explicit CharLiteral(char v) : value(v) {} };
struct Identifier : Expr { std::string name; explicit Identifier(std::string n) : name(std::move(n)) {} };
struct BinaryOp : Expr { std::string op; ExprPtr left, right; BinaryOp(std::string o, ExprPtr l, ExprPtr r) : op(std::move(o)), left(std::move(l)), right(std::move(r)) {} };
struct UnaryOp : Expr { std::string op; ExprPtr operand; UnaryOp(std::string o, ExprPtr e) : op(std::move(o)), operand(std::move(e)) {} };
struct Deref : Expr { ExprPtr ptr; explicit Deref(ExprPtr p) : ptr(std::move(p)) {} };
struct AddrOf : Expr { ExprPtr operand; explicit AddrOf(ExprPtr e) : operand(std::move(e)) {} };
struct CastExpr : Expr { Type targetType; ExprPtr expr; CastExpr(Type t, ExprPtr e) : targetType(std::move(t)), expr(std::move(e)) {} };
struct CallExpr : Expr { std::string callee; std::vector<ExprPtr> args; CallExpr(std::string name, std::vector<ExprPtr> a) : callee(std::move(name)), args(std::move(a)) {} };

struct Stmt { virtual ~Stmt() = default; };

struct AssignStmt : Stmt { ExprPtr lhs, rhs; AssignStmt(ExprPtr l, ExprPtr r) : lhs(std::move(l)), rhs(std::move(r)) {} };
struct CallStmt : Stmt { std::string name; std::vector<ExprPtr> args; CallStmt(std::string n, std::vector<ExprPtr> a) : name(std::move(n)), args(std::move(a)) {} };
struct TriggerStmt : Stmt { std::string ruleName; explicit TriggerStmt(std::string r) : ruleName(std::move(r)) {} };
struct IfStmt : Stmt { ExprPtr cond; std::vector<StmtPtr> thenBody, elseBody; IfStmt(ExprPtr c, std::vector<StmtPtr> t, std::vector<StmtPtr> e) : cond(std::move(c)), thenBody(std::move(t)), elseBody(std::move(e)) {} };
struct WhileStmt : Stmt { ExprPtr cond; std::vector<StmtPtr> body; WhileStmt(ExprPtr c, std::vector<StmtPtr> b) : cond(std::move(c)), body(std::move(b)) {} };
struct RetStmt : Stmt { ExprPtr value; explicit RetStmt(ExprPtr v = nullptr) : value(std::move(v)) {} };
struct HaltStmt : Stmt {};
struct NopStmt : Stmt {};
struct AsmStmt : Stmt { std::string code; explicit AsmStmt(std::string c) : code(std::move(c)) {} };
struct ExprStmt : Stmt { ExprPtr expr; explicit ExprStmt(ExprPtr e) : expr(std::move(e)) {} };

struct VarDeclStmt : Stmt {
    StorageClass storage;
    std::string name;
    Type type;
    ExprPtr initializer;
    VarDeclStmt(StorageClass s, std::string n, Type t, ExprPtr i = nullptr)
        : storage(s), name(std::move(n)), type(std::move(t)), initializer(std::move(i)) {}
};

struct GlobalVar {
    StorageClass storage;
    std::string name;
    Type type;
    ExprPtr initializer;
    GlobalVar(StorageClass s, std::string n, Type t, ExprPtr i = nullptr) : storage(s), name(std::move(n)), type(std::move(t)), initializer(std::move(i)) {}
};

struct Rule {
    std::string name;
    int priority = 128;
    bool atomic = false;
    ExprPtr condition;
    std::vector<StmtPtr> body;
    std::string every;
    Rule(std::string n, ExprPtr cond, std::vector<StmtPtr> b, int prio = 128, bool atm = false, std::string every = "")
        : name(std::move(n)), priority(prio), atomic(atm), condition(std::move(cond)), body(std::move(b)), every(std::move(every)) {}
};

struct Function {
    std::string name;
    std::vector<std::pair<std::string, Type>> params;
    Type returnType;
    std::vector<StmtPtr> body;
    Function(std::string n, std::vector<std::pair<std::string, Type>> p, Type r, std::vector<StmtPtr> b)
        : name(std::move(n)), params(std::move(p)), returnType(std::move(r)), body(std::move(b)) {}
};

struct Program {
    std::vector<GlobalVar> globals;
    std::vector<Rule> rules;
    std::vector<Function> functions;
    std::vector<StmtPtr> initBlock;
};

} // namespace ttek