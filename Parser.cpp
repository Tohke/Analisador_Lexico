#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
// Importa o Analisador Léxico (certifique-se de que o Analisador_lexico.h está na mesma pasta)
#include "Analisador_lexico.h"

using namespace std;

class Parser {
private:
    vector<Token> tokens;
    size_t pos;
public:
    Parser(vector<Token> t) : tokens(t), pos(0) {}

    Token peek() {
        if (pos >= tokens.size()) return tokens.back();
        return tokens[pos];
    }

    Token advance() {
        if (pos < tokens.size()) pos++;
        return tokens[pos - 1];
    }

    bool match(TokenType expected) {
        if (peek().type == expected) {
            advance();
            return true;
        }
        return false;
    }

    void error(string message) {
        Token t = peek();
        throw runtime_error(
            "Erro Sintatico: " + message +
            " (Token: " + t.lexeme + ") na linha " + to_string(t.line)
        );
    }

    void parseProgram() {
        while (peek().type != TokenType::T_EOF) {
            parseStatement();
        }
    }

    void parseAdditive() {
        parseTerm();
        while (peek().type == TokenType::T_PLUS || peek().type == TokenType::T_MINUS) {
            advance();
            parseTerm();
        }
    }

    void parseExpression() {
        parseAdditive();
        TokenType type = peek().type;
        if (type == TokenType::T_EQ || type == TokenType::T_LT || type == TokenType::T_GT) {
            advance();
            parseAdditive();
        }
    }

    void parseTerm() {
        parseFactor();
        while (peek().type == TokenType::T_MULT || peek().type == TokenType::T_DIV) {
            advance();
            parseFactor();
        }
    }

    void parseFactor() {
        TokenType type = peek().type;
        if (type == TokenType::T_NUM) {
            advance();
        }
        else if (type == TokenType::T_ID) {
            advance();
        }
        else if (type == TokenType::T_LPAREN) {
            advance();
            parseExpression();
            if (!match(TokenType::T_RPAREN)) {
                error("Esperado ')' apos expressao");
            }
        }
        else {
            error("Esperado numero, variavel ou '(' na expressao");
        }
    }

    void parseDeclaration() {
        match(TokenType::T_LET);
        if (peek().type == TokenType::T_MUT) {
            advance();
        }
        if (!match(TokenType::T_ID)) {
            error("Esperado nome da variavel apos 'let'");
        }
        if (peek().type == TokenType::T_COLON) {
            advance();
            if (!match(TokenType::T_TYPE)) {
                error("Esperado tipo da variavel (ex: i32) apos ':'");
            }
        }
        if (!match(TokenType::T_ASSIGN)) {
            error("Esperado '=' na declaracao");
        }
        parseExpression();
        if (!match(TokenType::T_SEMICOLON)) {
            error("Esperado ';' no final da instrucao");
        }
    }

    void parseStatement() {
        TokenType type = peek().type;
        if (type == TokenType::T_LET) {
            parseDeclaration();
        }
        else if (type == TokenType::T_PRINTLN) {
            parsePrintStmt();
        }
        else if (type == TokenType::T_IF) {
            parseIf();
        }
        else if (type == TokenType::T_WHILE) {
            parseWhile();
        }
        else if (type == TokenType::T_ID) {
            parseAssignment();
        }
        else {
            error("Comando invalido ou inesperado");
        }
    }

    void parseAssignment() {
        if (!match(TokenType::T_ID)) {
            error("Esperado nome da variavel para atribuicao");
        }
        if (!match(TokenType::T_ASSIGN)) {
            error("Esperado '=' apos o identificador");
        }
        parseExpression();
        if (!match(TokenType::T_SEMICOLON)) {
            error("Esperado ';' no final da instrucao de atribuicao");
        }
    }

    void parseIf() {
        match(TokenType::T_IF);
        parseExpression();
        if (!match(TokenType::T_LBRACE)) {
            error("Esperado '{' antes do bloco do if");
        }
        while (peek().type != TokenType::T_RBRACE && peek().type != TokenType::T_EOF) {
            parseStatement();
        }
        if (!match(TokenType::T_RBRACE)) {
            error("Esperado '}' apos o bloco do if");
        }
        if (peek().type == TokenType::T_ELSE) {
            advance();
            if (!match(TokenType::T_LBRACE)) {
                error("Esperado '{' antes do bloco do else");
            }
            while (peek().type != TokenType::T_RBRACE && peek().type != TokenType::T_EOF) {
                parseStatement();
            }
            if (!match(TokenType::T_RBRACE)) {
                error("Esperado '}' apos o bloco do else");
            }
        }
    }

    void parseWhile() {
        match(TokenType::T_WHILE);
        parseExpression();
        if (!match(TokenType::T_LBRACE)) {
            error("Esperado '{' antes do bloco do while");
        }
        while (peek().type != TokenType::T_RBRACE && peek().type != TokenType::T_EOF) {
            parseStatement();
        }
        if (!match(TokenType::T_RBRACE)) {
            error("Esperado '}' apos o bloco do while");
        }
    }

    void parsePrintStmt() {
        match(TokenType::T_PRINTLN);
        if (!match(TokenType::T_BANG)) {
            error("Esperado '!' apos println");
        }
        if (!match(TokenType::T_LPAREN)) {
            error("Esperado '(' apos println!");
        }
        if (!match(TokenType::T_STRING)) {
            error("Esperado string de formatacao (ex: \"{}\")");
        }
        if (peek().type == TokenType::T_COMMA) {
            advance();
            parseExpression();
        }
        if (!match(TokenType::T_RPAREN)) {
            error("Esperado ')' fechando o println!");
        }
        if (!match(TokenType::T_SEMICOLON)) {
            error("Esperado ';' no final da instrucao println!");
        }
    }
};

int main() {
    string code = R"(
    let num1: i32 = 10;     // Declaração usando identificador
    let num2 = 20;          // Declaração implicida

    let soma = num1 + num2;

    if soma == 30 {
        println!("{}", soma);
    }
    /*
    let numf = 2.5;         // Float correto
    let numf2 = 2.5.5       // Gera erro
    */
    )";

    Scanner scanner(code);

    try {
        vector<Token> tokens;
        Token token = scanner.nextToken();

        while (token.type != TokenType::T_EOF) {
            tokens.push_back(token);
            token = scanner.nextToken();
        }
        tokens.push_back(token); // Adiciona o EOF

        Parser parser(tokens);
        cout << "Iniciando a analise sintatica (Parsing)..." << endl;

        parser.parseProgram();

        cout << "Analise sintatica concluida com sucesso! Nenhum erro encontrado." << endl;
    }
    catch (exception& e) {
        cerr << e.what() << endl;
    }

    return 0;
}
