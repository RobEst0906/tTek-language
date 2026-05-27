#include "Parser.hpp"
#include <stdexcept>

namespace ttek {

Parser::Parser(const std::vector<Token>& tokens) : m_tokens(tokens) {}

const Token& Parser::peek() const { return m_tokens[m_pos]; }
const Token& Parser::advance() { return m_tokens[m_pos++]; }

bool Parser::check(TokenType type) const {
    return !isAtEnd() && peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

void Parser::expect(TokenType type, const std::string& errorMsg) {
    if (!match(type)) {
        std::string msg = errorMsg.empty() ? "Expected token type " + std::to_string(static_cast<int>(type)) : errorMsg;
        throw std::runtime_error(msg + " at line " + std::to_string(peek().line));
    }
}

bool Parser::isAtEnd() const {
    return m_pos >= m_tokens.size() || peek().type == TokenType::END_OF_FILE;
}

Type Parser::parseType() {
    if (match(TokenType::IDENTIFIER)) {
        std::string typeName = m_tokens[m_pos-1].lexeme;
        if (typeName == "u8")  return Type::makeSimple(TypeKind::U8);
        if (typeName == "u16") return Type::makeSimple(TypeKind::U16);
        if (typeName == "u32") return Type::makeSimple(TypeKind::U32);
        if (typeName == "u64") return Type::makeSimple(TypeKind::U64);
        if (typeName == "i8")  return Type::makeSimple(TypeKind::I8);
        if (typeName == "i16") return Type::makeSimple(TypeKind::I16);
        if (typeName == "i32") return Type::makeSimple(TypeKind::I32);
        if (typeName == "i64") return Type::makeSimple(TypeKind::I64);
        if (typeName == "f32") return Type::makeSimple(TypeKind::F32);
        if (typeName == "f64") return Type::makeSimple(TypeKind::F64);
        if (typeName == "ptr") {
            // FIX: не вызываем expect(IDENTIFIER) перед parseType() —
            // parseType() сама съест IDENTIFIER внутри себя
            Type inner = parseType();
            return Type::makePtr(std::make_unique<Type>(std::move(inner)));
        }
        throw std::runtime_error("Unknown type: " + typeName);
    }
    throw std::runtime_error("Expected type name at line " + std::to_string(peek().line));
}

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<Expr> Parser::parseAssignment() {
    auto expr = parseLogicalOr();
    if (match(TokenType::ASSIGN)) {
        auto lhs = std::move(expr);
        auto rhs = parseAssignment();
        return std::make_unique<BinaryOp>("=", std::move(lhs), std::move(rhs));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    while (match(TokenType::OR)) {
        auto rhs = parseLogicalAnd();
        expr = std::make_unique<BinaryOp>("||", std::move(expr), std::move(rhs));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto expr = parseComparison();
    while (match(TokenType::AND)) {
        auto rhs = parseComparison();
        expr = std::make_unique<BinaryOp>("&&", std::move(expr), std::move(rhs));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseComparison() {
    auto expr = parseAddition();
    while (match(TokenType::EQ) || match(TokenType::NEQ) ||
           match(TokenType::LT) || match(TokenType::GT) ||
           match(TokenType::LTE) || match(TokenType::GTE)) {
        std::string op = m_tokens[m_pos-1].lexeme;
        auto rhs = parseAddition();
        expr = std::make_unique<BinaryOp>(op, std::move(expr), std::move(rhs));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseAddition() {
    auto expr = parseTerm();
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        std::string op = m_tokens[m_pos-1].lexeme;
        auto rhs = parseTerm();
        expr = std::make_unique<BinaryOp>(op, std::move(expr), std::move(rhs));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseTerm() {
    auto expr = parseUnary();
    while (match(TokenType::STAR) || match(TokenType::DIV) || match(TokenType::MOD)) {
        std::string op = m_tokens[m_pos-1].lexeme;
        auto rhs = parseUnary();
        expr = std::make_unique<BinaryOp>(op, std::move(expr), std::move(rhs));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenType::MINUS) || match(TokenType::NOT) || match(TokenType::BIT_NOT)) {
        std::string op = m_tokens[m_pos-1].lexeme;
        auto operand = parseUnary();
        return std::make_unique<UnaryOp>(op, std::move(operand));
    }
    if (match(TokenType::STAR)) {
        auto ptr = parseUnary();
        return std::make_unique<Deref>(std::move(ptr));
    }
    if (match(TokenType::AMPERSAND)) {
        auto operand = parseUnary();
        return std::make_unique<AddrOf>(std::move(operand));
    }
    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    if (match(TokenType::INT_LIT)) {
        int64_t val = std::get<int64_t>(m_tokens[m_pos-1].value);
        return std::make_unique<IntLiteral>(val);
    }
    if (match(TokenType::FLOAT_LIT)) {
        double val = std::get<double>(m_tokens[m_pos-1].value);
        return std::make_unique<FloatLiteral>(val);
    }
    if (match(TokenType::STRING_LIT)) {
        std::string val = std::get<std::string>(m_tokens[m_pos-1].value);
        return std::make_unique<StringLiteral>(val);
    }
    if (match(TokenType::CHAR_LIT)) {
        char val = std::get<std::string>(m_tokens[m_pos-1].value)[0];
        return std::make_unique<CharLiteral>(val);
    }
    if (match(TokenType::IDENTIFIER)) {
        std::string name = m_tokens[m_pos-1].lexeme;
        auto ident = std::make_unique<Identifier>(name);
        return parseCallOrSuffix(std::move(ident));
    }
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    throw std::runtime_error("Unexpected token in expression: '" + peek().lexeme + "' at line " + std::to_string(peek().line));
}

std::unique_ptr<Expr> Parser::parseCallOrSuffix(std::unique_ptr<Expr> expr) {
    while (true) {
        if (match(TokenType::LPAREN)) {
            std::string callee;
            if (auto* id = dynamic_cast<Identifier*>(expr.get())) {
                callee = id->name;
            } else {
                throw std::runtime_error("Invalid call target");
            }
            std::vector<ExprPtr> args;
            if (!check(TokenType::RPAREN)) {
                args.push_back(parseExpression());
                while (match(TokenType::COMMA)) {
                    args.push_back(parseExpression());
                }
            }
            expect(TokenType::RPAREN, "Expected ')' after arguments");
            expr = std::make_unique<CallExpr>(callee, std::move(args));
        } else if (match(TokenType::LBRACKET)) {
            auto index = parseExpression();
            expect(TokenType::RBRACKET, "Expected ']' after index");
            expr = std::make_unique<BinaryOp>("[]", std::move(expr), std::move(index));
        } else if (match(TokenType::DOT)) {
            expect(TokenType::IDENTIFIER, "Expected field name after '.'");
            std::string field = m_tokens[m_pos-1].lexeme;
            expr = std::make_unique<BinaryOp>(".", std::move(expr), std::make_unique<Identifier>(field));
        } else if (match(TokenType::ARROW)) {
            expect(TokenType::IDENTIFIER, "Expected field name after '->'");
            std::string field = m_tokens[m_pos-1].lexeme;
            expr = std::make_unique<BinaryOp>("->", std::move(expr), std::make_unique<Identifier>(field));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    if (check(TokenType::MEM) || check(TokenType::REG)) {
        // FIX: использовали match() для обоих вариантов вместо advance() вслепую
        StorageClass storage;
        if (match(TokenType::REG)) {
            storage = StorageClass::REG;
        } else {
            match(TokenType::MEM); // съедаем 'mem'
            storage = StorageClass::MEM;
        }
        expect(TokenType::IDENTIFIER, "Expected variable name");
        std::string name = m_tokens[m_pos-1].lexeme;
        expect(TokenType::COLON, "Expected ':' after variable name");
        Type type = parseType();
        ExprPtr init = nullptr;
        if (match(TokenType::ASSIGN)) {
            init = parseExpression();
        }
        match(TokenType::SEMICOLON);
        return std::make_unique<VarDeclStmt>(storage, name, std::move(type), std::move(init));
    }

    if (match(TokenType::IF)) {
        expect(TokenType::LPAREN, "Expected '(' after 'if'");
        auto cond = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after if condition");
        auto thenBlock = parseBlock();
        std::vector<StmtPtr> elseBlock;
        if (match(TokenType::ELSE)) {
            if (check(TokenType::LBRACE)) {
                elseBlock = parseBlock();
            } else {
                elseBlock.push_back(parseStatement());
            }
        }
        return std::make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
    }
    if (match(TokenType::WHILE)) {
        expect(TokenType::LPAREN, "Expected '(' after 'while'");
        auto cond = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after while condition");
        auto body = parseBlock();
        return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
    }
    if (match(TokenType::RET)) {
        ExprPtr val = nullptr;
        if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE)) {
            val = parseExpression();
        }
        match(TokenType::SEMICOLON);
        return std::make_unique<RetStmt>(std::move(val));
    }
    if (match(TokenType::HALT)) {
        match(TokenType::SEMICOLON);
        return std::make_unique<HaltStmt>();
    }
    if (match(TokenType::NOP)) {
        match(TokenType::SEMICOLON);
        return std::make_unique<NopStmt>();
    }
    if (match(TokenType::TRIGGER)) {
        expect(TokenType::IDENTIFIER, "Expected rule name after 'trigger'");
        std::string name = m_tokens[m_pos-1].lexeme;
        match(TokenType::SEMICOLON);
        return std::make_unique<TriggerStmt>(name);
    }

    auto expr = parseExpression();
    match(TokenType::SEMICOLON);
    if (auto* bin = dynamic_cast<BinaryOp*>(expr.get()); bin && bin->op == "=") {
        return std::make_unique<AssignStmt>(std::move(bin->left), std::move(bin->right));
    }
    if (auto* call = dynamic_cast<CallExpr*>(expr.get())) {
        std::vector<ExprPtr> args;
        for (auto& a : call->args) args.push_back(std::move(a));
        return std::make_unique<CallStmt>(call->callee, std::move(args));
    }
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::vector<std::unique_ptr<Stmt>> Parser::parseBlock() {
    std::vector<StmtPtr> stmts;
    if (match(TokenType::LBRACE)) {
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            stmts.push_back(parseStatement());
        }
        expect(TokenType::RBRACE, "Expected '}' after block");
    } else {
        stmts.push_back(parseStatement());
    }
    return stmts;
}

std::unique_ptr<GlobalVar> Parser::parseGlobalVar() {
    StorageClass storage = StorageClass::MEM;
    if (match(TokenType::REG)) storage = StorageClass::REG;
    else expect(TokenType::MEM, "Global variable must start with 'mem' or 'reg'");

    expect(TokenType::IDENTIFIER, "Expected variable name");
    std::string name = m_tokens[m_pos-1].lexeme;
    expect(TokenType::COLON, "Expected ':' after variable name");
    Type type = parseType();
    ExprPtr init = nullptr;
    if (match(TokenType::ASSIGN)) {
        init = parseExpression();
    }
    match(TokenType::SEMICOLON);
    return std::make_unique<GlobalVar>(storage, name, std::move(type), std::move(init));
}

std::unique_ptr<Rule> Parser::parseRule() {
    expect(TokenType::RULE, "Expected 'rule'");
    std::string name;
    if (check(TokenType::IDENTIFIER)) {
        name = advance().lexeme;
    }
    int priority = 128;
    bool atomic = false;
    while (match(TokenType::PRIORITY)) {
        expect(TokenType::INT_LIT, "Priority value expected");
        priority = static_cast<int>(std::get<int64_t>(m_tokens[m_pos-1].value));
    }
    if (match(TokenType::ATOMIC)) atomic = true;

    std::string every;
    if (match(TokenType::EVERY)) {
        expect(TokenType::STRING_LIT, "Time string expected after 'every'");
        every = std::get<std::string>(m_tokens[m_pos-1].value);
    }

    expect(TokenType::WHEN, "Expected 'when' in rule");
    auto cond = parseExpression();
    auto body = parseBlock();
    return std::make_unique<Rule>(name, std::move(cond), std::move(body), priority, atomic, every);
}

std::unique_ptr<Function> Parser::parseFunction() {
    expect(TokenType::FN, "Expected 'fn'");
    expect(TokenType::IDENTIFIER, "Function name expected");
    std::string name = m_tokens[m_pos-1].lexeme;

    expect(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<std::pair<std::string, Type>> params;
    if (!check(TokenType::RPAREN)) {
        do {
            expect(TokenType::IDENTIFIER, "Parameter name expected");
            std::string pname = m_tokens[m_pos-1].lexeme;
            expect(TokenType::COLON, "Expected ':' after parameter name");
            Type ptype = parseType();
            params.emplace_back(pname, std::move(ptype));
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN, "Expected ')' after parameters");

    Type returnType = Type::makeSimple(TypeKind::VOID);
    if (match(TokenType::COLON)) {
        returnType = parseType();
    }

    auto body = parseBlock();
    return std::make_unique<Function>(name, std::move(params), std::move(returnType), std::move(body));
}

std::vector<std::unique_ptr<Stmt>> Parser::parseInitBlock() {
    expect(TokenType::INIT, "Expected 'init'");
    return parseBlock();
}

Program Parser::parse() {
    Program program;
    while (!isAtEnd()) {
        if (check(TokenType::MEM) || check(TokenType::REG)) {
            program.globals.push_back(std::move(*parseGlobalVar()));
        } else if (check(TokenType::RULE)) {
            program.rules.push_back(std::move(*parseRule()));
        } else if (check(TokenType::FN)) {
            program.functions.push_back(std::move(*parseFunction()));
        } else if (check(TokenType::INIT)) {
            // Parse init block: mem/reg go to globals, other stmts to initBlock
            expect(TokenType::INIT, "Expected 'init'");
            expect(TokenType::LBRACE, "Expected '{' after 'init'");
            while (!check(TokenType::RBRACE) && !isAtEnd()) {
                if (check(TokenType::MEM) || check(TokenType::REG)) {
                    program.globals.push_back(std::move(*parseGlobalVar()));
                } else {
                    program.initBlock.push_back(parseStatement());
                }
            }
            expect(TokenType::RBRACE, "Expected '}' after init block");
        } else {
            throw std::runtime_error("Unexpected top-level token: '" + peek().lexeme + "' at line " + std::to_string(peek().line));
        }
    }
    return program;
}

} // namespace ttek