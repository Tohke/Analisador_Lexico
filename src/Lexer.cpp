#include "Lexer.h"

#include <cctype>
#include <stdexcept>

using namespace std;

Scanner::Scanner(string source) : input(std::move(source)), pos(0), line(1) {
    keywords["if"] = TokenType::T_IF;
    keywords["else"] = TokenType::T_ELSE;
    keywords["while"] = TokenType::T_WHILE;
    keywords["for"] = TokenType::T_FOR;
    keywords["println"] = TokenType::T_PRINTLN;
    keywords["let"] = TokenType::T_LET;
    keywords["mut"] = TokenType::T_MUT;
    keywords["fn"] = TokenType::T_FN;
    keywords["match"] = TokenType::T_MATCH;
    keywords["_"] = TokenType::T_UL;
    keywords["i32"] = TokenType::T_TYPE;
    keywords["u32"] = TokenType::T_TYPE;
    keywords["f32"] = TokenType::T_TYPE;
    keywords["bool"] = TokenType::T_TYPE;
    keywords["char"] = TokenType::T_TYPE;
    keywords["str"]  = TokenType::T_TYPE;
}

char Scanner::peek() {
    if (pos >= input.length()) return '\0';
    return input[pos];
}

char Scanner::next() {
    char c = peek();
    if (c != '\0') pos++;
    return c;
}

void Scanner::skipWhitespace() {
    while (isspace(peek())) {
        if (next() == '\n') line++;
    }
}

void Scanner::skipComment() {
    while (peek() != '\n' && peek() != '\0') next();
}

void Scanner::skipMultilineComment() {
    while (true) {
        if (peek() == '\0') {
            throw runtime_error("Erro Lexico: comentario multilinha nao fechado na linha " + to_string(line));
        }
        if (peek() == '\n') line++;
        if (peek() == '*') {
            next();
            if (peek() == '/') {
                next();
                break;
            }
            continue;
        }
        next();
    }
}

Token Scanner::scanNumber(char start) {
    string buffer;
    buffer += start;

    char c = peek();
    bool isFloat = false;

    while (isdigit(c) || c == '.') {
        if (c == '.') {
            if (isFloat) {
                throw runtime_error("Erro Lexico: Float com dois pontos na Linha " + to_string(line));
            }
            isFloat = true;
        }
        buffer += next();
        c = peek();
    }
    return Token(TokenType::T_NUM, buffer, line);
}

Token Scanner::scanString() {
    string buffer = "\"";
    while (peek() != '"' && peek() != '\0') {
        if (peek() == '}') {
            throw runtime_error("Erro Lexico: placeholder invalido na linha " + to_string(line));
        }
        if (peek() == '{') {
            buffer += next();
            if (peek() == '}') {
                buffer += next();
                continue;
            }
            throw runtime_error("Erro Lexico: placeholder invalido na linha " + to_string(line));
        }
        buffer += next();
    }
    if (peek() == '"') {
        buffer += next();
    } else {
        throw runtime_error("Erro Lexico: String nao fechada na linha " + to_string(line));
    }
    return Token(TokenType::T_STRING, buffer, line);
}

Token Scanner::scanIdentifier(char start) {
    string buffer;
    buffer += start;
    while (isalnum(peek()) || peek() == '_') {
        buffer += next();
    }
    if (keywords.count(buffer)) {
        return Token(keywords[buffer], buffer, line);
    }
    return Token(TokenType::T_ID, buffer, line);
}

Token Scanner::nextToken() {
    skipWhitespace();
    if (pos >= input.length()) return Token(TokenType::T_EOF, "", line);

    char c = next();
    if (isdigit(c)) return scanNumber(c);
    if (isalpha(c) || c == '_') return scanIdentifier(c);
    if (c == '"') return scanString();

    switch (c) {
    case '+': return Token(TokenType::T_PLUS, "+", line);
    case '-':
        if (peek() == '>') {
            next();
            return Token(TokenType::T_ARROW, "->", line);
        }
        return Token(TokenType::T_MINUS, "-", line);
    case '*': return Token(TokenType::T_MULT, "*", line);
    case '/':
        if (peek() == '/') {
            next();
            skipComment();
            return nextToken();
        }
        if (peek() == '*') {
            next();
            skipMultilineComment();
            return nextToken();
        }
        return Token(TokenType::T_DIV, "/", line);
    case '=':
        if (peek() == '=') {
            next();
            return Token(TokenType::T_EQ, "==", line);
        }
        if (peek() == '>') {
            next();
            return Token(TokenType::T_FAT_ARROW, "=>", line);
        }
        return Token(TokenType::T_ASSIGN, "=", line);
    case '!':
        if (peek() == '=') {
            next();
            return Token(TokenType::T_NE, "!=", line);
        } else return Token(TokenType::T_BANG, "!", line);
    case '&':
        if (peek() == '&') {
            next();
            return Token(TokenType::T_AND, "&&", line);
        } else return Token(TokenType::T_REF, "&", line);
    case '|':
        if (peek() == '|') {
            next();
            return Token(TokenType::T_OR, "||", line);
        } else {
            throw runtime_error("Erro Lexico: caractere Esperado: |, caracter encontrado" + to_string(peek()) + " na linha " + to_string(line));
        }
    case ':': return Token(TokenType::T_COLON, ":", line);
    case ',': return Token(TokenType::T_COMMA, ",", line);
    case '<':
        if (peek() == '=') {
            next();
            return Token(TokenType::T_LE, "<=", line);
        } else return Token(TokenType::T_LT, "<", line);
    case '>':
        if (peek() == '=') {
            next();
            return Token(TokenType::T_GE, ">=", line);
        } else return Token(TokenType::T_GT, ">", line);

    case '(': return Token(TokenType::T_LPAREN, "(", line);
    case ')': return Token(TokenType::T_RPAREN, ")", line);
    case '{': return Token(TokenType::T_LBRACE, "{", line);
    case '}': return Token(TokenType::T_RBRACE, "}", line);
    case ';': return Token(TokenType::T_SEMICOLON, ";", line);
    default:
        throw runtime_error("Erro Lexico: caractere invalido '" + string(1, c) + "' na linha " + to_string(line));
    }
}
