#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include <string>

#include "Token.h"
#include "AST.h"

// =====================================================================
// PANIC MODE
// =====================================================================
class panicMode : public std::runtime_error {
public:
    explicit panicMode(const std::string& mensagem) : std::runtime_error(mensagem) {}
};

// =====================================================================
// CLASSE PARSER (RETORNANDO NÓS DA AST)
// =====================================================================
class Parser {
private:
    std::vector<Token> tokens;
    size_t pos;

    void synchronize();

public:
    explicit Parser(std::vector<Token> t);

    Token peek();
    Token advance();
    bool match(TokenType expected);
    void error(std::string message);

    std::shared_ptr<ASTNode> parseProgram();
    std::shared_ptr<ASTNode> parseStatement();
    std::shared_ptr<ASTNode> parseDeclaration();
    std::shared_ptr<ASTNode> parseAssignment();
    std::shared_ptr<ASTNode> parseIf();
    std::shared_ptr<ASTNode> parseWhile();
    std::shared_ptr<ASTNode> parseFor();
    std::shared_ptr<ASTNode> parsePrintStmt();

    std::shared_ptr<ASTNode> parseExpression();
    std::shared_ptr<ASTNode> parseAdditive();
    std::shared_ptr<ASTNode> parseTerm();
    std::shared_ptr<ASTNode> parseFactor();
};

#endif // PARSER_H
