#pragma once
#include "Token.hpp"
#include <string>
#include <vector>
#include <unordered_map>

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    void skipWhitespaceAndComments();
    Token readNumber();
    Token readString();
    Token readIdentifierOrKeyword();
    Token readOperator();
    char peek() const;
    char advance();
    bool isAtEnd() const;

    std::string m_source;
    size_t m_pos = 0;
    int m_line = 1;
    int m_column = 1;
    std::vector<Token> m_tokens;

    static const std::unordered_map<std::string, TokenType> s_keywords;
};