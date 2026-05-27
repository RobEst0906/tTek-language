#pragma once
#include "AST.hpp"
#include "../lexer/Token.hpp"
#include <vector>
#include <memory>
#include <string>

namespace ttek {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    Program parse();

private:
    const std::vector<Token>& m_tokens;
    size_t m_pos = 0;

    const Token& peek() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    void expect(TokenType type, const std::string& errorMsg = "");
    bool isAtEnd() const;

    Type parseType();
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseComparison();
    std::unique_ptr<Expr> parseAddition();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parseCallOrSuffix(std::unique_ptr<Expr> expr);
    std::unique_ptr<Stmt> parseStatement();
    std::vector<std::unique_ptr<Stmt>> parseBlock();
    std::unique_ptr<GlobalVar> parseGlobalVar();
    std::unique_ptr<Rule> parseRule();
    std::unique_ptr<Function> parseFunction();
    std::vector<std::unique_ptr<Stmt>> parseInitBlock();
};

} // namespace ttek