#pragma once
#include <string>
#include <variant>
#include <cstdint>

enum class TokenType {
    RULE, WHEN, EVERY, PRIORITY, ATOMIC,
    INIT, MEM, REG, FN, CALL, TRIGGER,
    AS, HALT, NOP, IF, ELSE, WHILE, RET,
    IDENTIFIER, INT_LIT, FLOAT_LIT, STRING_LIT, CHAR_LIT,
    LBRACE, RBRACE, LPAREN, RPAREN, LBRACKET, RBRACKET,
    SEMICOLON, COMMA, COLON, ARROW,
    ASSIGN, STAR, AMPERSAND,
    PLUS, MINUS, DIV, MOD,
    AND, OR, NOT,
    EQ, NEQ, LT, GT, LTE, GTE,
    BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, SHL, SHR,
    DOT,
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    std::variant<std::monostate, int64_t, double, std::string> value;
};