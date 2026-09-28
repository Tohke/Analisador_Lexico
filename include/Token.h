#ifndef TOKEN_H
#define TOKEN_H

#include <string>

// Enumeração que representa todos os tipos de tokens
enum class TokenType {
    T_IF,
    T_ELSE,
    T_WHILE,
    T_PRINTLN,
    T_FN,
    T_LET,
    T_MUT,
    T_MATCH,
    T_UL,
    T_BANG,
    T_REF,
    T_COLON,
    T_ARROW,
    T_FAT_ARROW,
    T_STRING,
    T_COMMA,
    T_ID,
    T_NUM,
    T_TYPE,
    T_ASSIGN,
    T_EQ,
    T_PLUS,
    T_MINUS,
    T_MULT,
    T_DIV,
    T_LT,
    T_GT,
    T_LPAREN,
    T_RPAREN,
    T_LBRACE,
    T_RBRACE,
    T_SEMICOLON,
    T_EOF,
    T_LE,
    T_GE,
    T_NE,
    T_AND,
    T_OR
};

// Estrutura que representa um token
struct Token {
    TokenType type;
    std::string lexeme;
    int line;

    Token(TokenType t, std::string l, int ln) : type(t), lexeme(std::move(l)), line(ln) {}
};

// Converte um TokenType para sua representação textual (usado em logs/depuração)
std::string tokenTypeToString(TokenType type);

#endif // TOKEN_H
