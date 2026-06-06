#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <cctype>
#include <iomanip>
#include <vector>

using namespace std;

// Enumeração que representa todos os tipos de tokens
enum class TokenType {
    T_IF, T_ELSE, T_WHILE, T_PRINTLN,
    T_FN, T_LET, T_MUT, T_MATCH, T_UL, T_BANG, T_REF, T_COLON,
    T_ARROW, T_FAT_ARROW, T_STRING, T_COMMA,
    T_ID, T_NUM, T_TYPE,
    T_ASSIGN, T_EQ,
    T_PLUS, T_MINUS, T_MULT, T_DIV,
    T_LT, T_GT,
    T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_SEMICOLON,
    T_EOF
};

// Estrutura que representa um token
struct Token {
    TokenType type;
    string lexeme;
    int line;

    Token(TokenType t, string l, int ln) : type(t), lexeme(l), line(ln) {}
};

// Classe do Scanner
class Scanner {
private:
    string input;
    size_t pos;
    int line;
    unordered_map<string, TokenType> keywords;

public:
    Scanner(string source) : input(source), pos(0), line(1) {
        keywords["if"] = TokenType::T_IF;
        keywords["else"] = TokenType::T_ELSE;
        keywords["while"] = TokenType::T_WHILE;
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

    char peek() {
        if (pos >= input.length()) return '\0';
        return input[pos];
    }

    char next() {
        char c = peek();
        if (c != '\0') pos++;
        return c;
    }

    void skipWhitespace() {
        while (isspace(peek())) {
            if (next() == '\n') line++;
        }
    }

    void skipComment() {
        while (peek() != '\n' && peek() != '\0') next();
    }

    void skipMultilineComment() {
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

    Token scanNumber(char start) {
        string buffer;
        buffer += start;
        char c = start;
        bool isFloat = false;

        while (isdigit(c) || c == '.') {
            if(c == '.'){
                if(isFloat){
                    throw runtime_error("Erro Lexico: Float com dois pontos na Linha " + to_string(line));
                }
                isFloat = true;
            }
            buffer += next();
            c = peek();
        }
        return Token(TokenType::T_NUM, buffer, line);
    }

    Token scanString(){
        string buffer = "\"";
        while (peek() != '"' && peek() != '\0') {
            if (peek() == '}'){
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

    Token scanIdentifier(char start) {
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

    Token nextToken() {
        skipWhitespace();
        if (pos >= input.length()) return Token(TokenType::T_EOF, "", line);

        char c = next();
        if (isdigit(c)) return scanNumber(c);
        if (isalpha(c) || c == '_') return scanIdentifier(c);
        if(c == '"') return scanString();

        switch (c) {
        case '+': return Token(TokenType::T_PLUS, "+", line);
        case '-':
            if (peek() == '>'){
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
            if (peek() == '>'){
                next();
                return Token(TokenType::T_FAT_ARROW, "=>", line);
            }
            return Token(TokenType::T_ASSIGN, "=", line);
        case '!': return Token(TokenType::T_BANG, "!", line);
        case '&': return Token(TokenType::T_REF, "&", line);
        case ':': return Token(TokenType::T_COLON, ":", line); // Corrigido de T_REF para T_COLON
        case ',': return Token(TokenType::T_COMMA, ",", line);
        case '<': return Token(TokenType::T_LT, "<", line);
        case '>': return Token(TokenType::T_GT, ">", line);
        case '(': return Token(TokenType::T_LPAREN, "(", line);
        case ')': return Token(TokenType::T_RPAREN, ")", line);
        case '{': return Token(TokenType::T_LBRACE, "{", line);
        case '}': return Token(TokenType::T_RBRACE, "}", line);
        case ';': return Token(TokenType::T_SEMICOLON, ";", line);
        default:
            throw runtime_error("Erro Lexico: caractere invalido '" + string(1, c) + "' na linha " + to_string(line));
        }
    }
};

inline string tokenTypeToString(TokenType type) {
    switch(type) {
        case TokenType::T_IF: return "T_IF";
        case TokenType::T_ELSE: return "T_ELSE";
        case TokenType::T_WHILE: return "T_WHILE";
        case TokenType::T_PRINTLN: return "T_PRINTLN";
        case TokenType::T_ID: return "T_ID";
        case TokenType::T_NUM: return "T_NUM";
        case TokenType::T_FN: return "T_FN";
        case TokenType::T_LET: return "T_LET";
        case TokenType::T_MUT: return "T_MUT";
        case TokenType::T_MATCH: return "T_MATCH";
        case TokenType::T_UL: return "T_UL";
        case TokenType::T_BANG: return "T_BANG";
        case TokenType::T_REF: return "T_REF";
        case TokenType::T_COLON: return "T_COLON";
        case TokenType::T_ARROW: return "T_ARROW";
        case TokenType::T_FAT_ARROW: return "T_FAT_ARROW";
        case TokenType::T_TYPE: return "T_TYPE";
        case TokenType::T_STRING: return "T_STRING";
        case TokenType::T_COMMA: return "T_COMMA";
        case TokenType::T_ASSIGN: return "T_ASSIGN";
        case TokenType::T_EQ: return "T_EQ";
        case TokenType::T_PLUS: return "T_PLUS";
        case TokenType::T_MINUS: return "T_MINUS";
        case TokenType::T_MULT: return "T_MULT";
        case TokenType::T_DIV: return "T_DIV";
        case TokenType::T_LT: return "T_LT";
        case TokenType::T_GT: return "T_GT";
        case TokenType::T_LPAREN: return "T_LPAREN";
        case TokenType::T_RPAREN: return "T_RPAREN";
        case TokenType::T_LBRACE: return "T_LBRACE";
        case TokenType::T_RBRACE: return "T_RBRACE";
        case TokenType::T_SEMICOLON: return "T_SEMICOLON";
        case TokenType::T_EOF: return "T_EOF";
        default: return "UNKNOWN";
    }

}
