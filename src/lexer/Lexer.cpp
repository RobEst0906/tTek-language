#include "Lexer.hpp"
#include <cctype>
#include <stdexcept>

const std::unordered_map<std::string, TokenType> Lexer::s_keywords = {
    {"rule", TokenType::RULE},
    {"when", TokenType::WHEN},
    {"every", TokenType::EVERY},
    {"priority", TokenType::PRIORITY},
    {"atomic", TokenType::ATOMIC},
    {"init", TokenType::INIT},
    {"mem", TokenType::MEM},
    {"reg", TokenType::REG},
    {"fn", TokenType::FN},
    {"call", TokenType::CALL},
    {"trigger", TokenType::TRIGGER},
    {"as", TokenType::AS},
    {"halt", TokenType::HALT},
    {"nop", TokenType::NOP},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"ret", TokenType::RET},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT}
};

Lexer::Lexer(const std::string& source) : m_source(source) {}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;

        char c = peek();
        if (std::isdigit(c)) {
            m_tokens.push_back(readNumber());
        } else if (c == '"') {
            m_tokens.push_back(readString());
        } else if (std::isalpha(c) || c == '_') {
            m_tokens.push_back(readIdentifierOrKeyword());
        } else {
            Token tok = readOperator();
            if (tok.type != TokenType::UNKNOWN) {
                m_tokens.push_back(tok);
            } else {
                advance();
            }
        }
    }
    m_tokens.push_back({TokenType::END_OF_FILE, "", m_line, m_column});
    return m_tokens;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance();
        } else if (c == '/') {
            if (m_pos + 1 < m_source.size() && m_source[m_pos+1] == '/') {
                while (!isAtEnd() && peek() != '\n') advance();
            } else if (m_pos + 1 < m_source.size() && m_source[m_pos+1] == '*') {
                advance(); advance();
                while (!isAtEnd() && !(peek() == '*' && m_pos+1 < m_source.size() && m_source[m_pos+1] == '/')) {
                    advance();
                }
                if (!isAtEnd()) {
                    advance(); advance();
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

Token Lexer::readNumber() {
    int start_col = m_column;
    std::string num;
    bool is_float = false;
    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.' || peek() == 'x' || peek() == 'X' ||
           (peek() >= 'a' && peek() <= 'f') || (peek() >= 'A' && peek() <= 'F'))) {
        char c = advance();
        num += c;
        if (c == '.') is_float = true;
    }
    TokenType type = is_float ? TokenType::FLOAT_LIT : TokenType::INT_LIT;
    if (is_float) {
        return {type, num, m_line, start_col, std::stod(num)};
    } else {
        int64_t val = 0;
        if (num.size() > 2 && num[0] == '0' && (num[1] == 'x' || num[1] == 'X')) {
            val = std::stoll(num, nullptr, 16);
        } else {
            val = std::stoll(num);
        }
        return {type, num, m_line, start_col, val};
    }
}

Token Lexer::readString() {
    int start_col = m_column;
    advance();
    std::string str;
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\') {
            advance();
            if (!isAtEnd()) {
                char esc = advance();
                switch (esc) {
                    case 'n': str += '\n'; break;
                    case 't': str += '\t'; break;
                    case '"': str += '"'; break;
                    case '\\': str += '\\'; break;
                    default: str += esc; break;
                }
            }
        } else {
            str += advance();
        }
    }
    if (isAtEnd()) throw std::runtime_error("Unterminated string literal");
    advance();
    return {TokenType::STRING_LIT, str, m_line, start_col, str};
}

Token Lexer::readIdentifierOrKeyword() {
    int start_col = m_column;
    std::string ident;
    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_')) {
        ident += advance();
    }
    auto it = s_keywords.find(ident);
    TokenType type = (it != s_keywords.end()) ? it->second : TokenType::IDENTIFIER;
    return {type, ident, m_line, start_col};
}

Token Lexer::readOperator() {
    int start_col = m_column;
    char c = peek();
    switch (c) {
        case '{': advance(); return {TokenType::LBRACE, "{", m_line, start_col};
        case '}': advance(); return {TokenType::RBRACE, "}", m_line, start_col};
        case '(': advance(); return {TokenType::LPAREN, "(", m_line, start_col};
        case ')': advance(); return {TokenType::RPAREN, ")", m_line, start_col};
        case '[': advance(); return {TokenType::LBRACKET, "[", m_line, start_col};
        case ']': advance(); return {TokenType::RBRACKET, "]", m_line, start_col};
        case ';': advance(); return {TokenType::SEMICOLON, ";", m_line, start_col};
        case ',': advance(); return {TokenType::COMMA, ",", m_line, start_col};
        case ':': advance(); return {TokenType::COLON, ":", m_line, start_col};
        case '=':
            advance();
            if (!isAtEnd() && peek() == '=') {
                advance();
                return {TokenType::EQ, "==", m_line, start_col};
            }
            return {TokenType::ASSIGN, "=", m_line, start_col};
        case '*': advance(); return {TokenType::STAR, "*", m_line, start_col};
        case '&':
            advance();
            if (!isAtEnd() && peek() == '&') {
                advance();
                return {TokenType::AND, "&&", m_line, start_col};
            }
            return {TokenType::AMPERSAND, "&", m_line, start_col};
        case '+': advance(); return {TokenType::PLUS, "+", m_line, start_col};
        case '-':
            advance();
            if (!isAtEnd() && peek() == '>') {
                advance();
                return {TokenType::ARROW, "->", m_line, start_col};
            }
            return {TokenType::MINUS, "-", m_line, start_col};
        case '/': advance(); return {TokenType::DIV, "/", m_line, start_col};
        case '%': advance(); return {TokenType::MOD, "%", m_line, start_col};
        case '!':
            advance();
            if (!isAtEnd() && peek() == '=') {
                advance();
                return {TokenType::NEQ, "!=", m_line, start_col};
            }
            return {TokenType::NOT, "!", m_line, start_col};
        case '<':
            advance();
            if (!isAtEnd() && peek() == '=') {
                advance();
                return {TokenType::LTE, "<=", m_line, start_col};
            } else if (!isAtEnd() && peek() == '<') {
                advance();
                return {TokenType::SHL, "<<", m_line, start_col};
            }
            return {TokenType::LT, "<", m_line, start_col};
        case '>':
            advance();
            if (!isAtEnd() && peek() == '=') {
                advance();
                return {TokenType::GTE, ">=", m_line, start_col};
            } else if (!isAtEnd() && peek() == '>') {
                advance();
                return {TokenType::SHR, ">>", m_line, start_col};
            }
            return {TokenType::GT, ">", m_line, start_col};
        case '|':
            advance();
            if (!isAtEnd() && peek() == '|') {
                advance();
                return {TokenType::OR, "||", m_line, start_col};
            }
            return {TokenType::BIT_OR, "|", m_line, start_col};
        case '^': advance(); return {TokenType::BIT_XOR, "^", m_line, start_col};
        case '~': advance(); return {TokenType::BIT_NOT, "~", m_line, start_col};
        case '.': advance(); return {TokenType::DOT, ".", m_line, start_col};
        default:
            return {TokenType::UNKNOWN, std::string(1, c), m_line, start_col};
    }
}

char Lexer::peek() const {
    return isAtEnd() ? '\0' : m_source[m_pos];
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return m_pos >= m_source.size();
}