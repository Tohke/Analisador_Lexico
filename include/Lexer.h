#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <unordered_map>

#include "Token.h"

// Classe do Scanner (Analisador Léxico)
class Scanner {
private:
    std::string input;
    size_t pos;
    int line;
    std::unordered_map<std::string, TokenType> keywords;

    void skipWhitespace();
    void skipComment();
    void skipMultilineComment();

    Token scanNumber(char start);
    Token scanString();
    Token scanIdentifier(char start);

public:
    explicit Scanner(std::string source);

    char peek();
    char next();

    Token nextToken();
};

#endif // LEXER_H
