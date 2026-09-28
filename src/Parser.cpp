#include "Parser.h"

#include <iostream>

using namespace std;

Parser::Parser(vector<Token> t) : tokens(std::move(t)), pos(0) {}

Token Parser::peek() {
    if (pos >= tokens.size()) return tokens.back();
    return tokens[pos];
}

Token Parser::advance() {
    if (pos < tokens.size()) pos++;
    return tokens[pos - 1];
}

bool Parser::match(TokenType expected) {
    if (peek().type == expected) {
        advance();
        return true;
    }
    return false;
}

void Parser::error(string message) {
    Token t = peek();
    throw panicMode(
        "Erro Sintatico: " + message +
        " (Token: '" + t.lexeme + "') na linha " + to_string(t.line)
    );
}

// Motor do Panic Mode -> Recupera o parser de um estado de erro
void Parser::synchronize() {
    advance(); // Consome o token que causou o problema inicial

    while (peek().type != TokenType::T_EOF) {
        // Se o token anterior foi um ponto e vírgula, a próxima instrução é segura
        if (tokens[pos - 1].type == TokenType::T_SEMICOLON) {
            return;
        }

        // Se o token atual é o início de uma nova estrutura, é seguro recomeçar
        switch (peek().type) {
            case TokenType::T_LET:
            case TokenType::T_IF:
            case TokenType::T_WHILE:
            case TokenType::T_PRINTLN:
            case TokenType::T_RBRACE:
                return;
            default:
                advance(); // O token não é seguro, descarta e continua procurando
        }
    }
}

shared_ptr<ASTNode> Parser::parseProgram() {
    auto node = make_shared<ASTNode>("Program");
    bool hasErrors = false;

    while (peek().type != TokenType::T_EOF) {
        try {
            // Tenta fazer o parse da instrução e adicionar à AST
            node->addChild(parseStatement());
        } catch (panicMode& e) { // Ativa o Panic Mode
            // Imprime o erro no console
            cerr << e.what() << "\n";
            hasErrors = true;

            // Descarta o "lixo" até o próximo ponto seguro
            synchronize();
        }
    }

    if (hasErrors) {
        cout << "\n[Aviso] A AST foi gerada parcialmente devido a erros sintaticos.\n";
    }

    return node;
}

shared_ptr<ASTNode> Parser::parseAdditive() {
    auto left = parseTerm();
    while (peek().type == TokenType::T_PLUS || peek().type == TokenType::T_MINUS) {
        Token op = advance();
        auto node = make_shared<ASTNode>("Op: " + op.lexeme);
        node->addChild(left);
        node->addChild(parseTerm());
        left = node;
    }
    return left;
}

shared_ptr<ASTNode> Parser::parseExpression() {
    auto left = parseAdditive();
    TokenType type = peek().type;
    if (type == TokenType::T_EQ || type == TokenType::T_LT || type == TokenType::T_GT
        || type == TokenType::T_NE || type == TokenType::T_GE || type == TokenType::T_LE
        || type == TokenType::T_AND || type == TokenType::T_OR || type == TokenType::T_BANG) {
        Token op = advance();
        auto node = make_shared<ASTNode>("Condition: " + op.lexeme);
        node->addChild(left);
        node->addChild(parseAdditive());
        left = node;
    }
    return left;
}

shared_ptr<ASTNode> Parser::parseTerm() {
    auto left = parseFactor();
    while (peek().type == TokenType::T_MULT || peek().type == TokenType::T_DIV) {
        Token op = advance();
        auto node = make_shared<ASTNode>("Op: " + op.lexeme);
        node->addChild(left);
        node->addChild(parseFactor());
        left = node;
    }
    return left;
}

shared_ptr<ASTNode> Parser::parseFactor() {
    TokenType type = peek().type;
    if (type == TokenType::T_NUM) {
        return make_shared<ASTNode>("LiteralNumber", advance().lexeme);
    }
    else if (type == TokenType::T_ID) {
        return make_shared<ASTNode>("Variable", advance().lexeme);
    }
    else if (type == TokenType::T_LPAREN) {
        advance();
        auto node = parseExpression();
        if (!match(TokenType::T_RPAREN)) {
            error("Esperado ')' apos expressao");
        }
        return node;
    }
    else {
        error("Esperado numero, variavel ou '(' na expressao");
        return nullptr;
    }
}

shared_ptr<ASTNode> Parser::parseDeclaration() {
    // Agora o pai é a ação de atribuição "LetAssignment"
    auto node = make_shared<ASTNode>("LetAssignment");
    match(TokenType::T_LET);

    string varModifier = "";
    if (peek().type == TokenType::T_MUT) {
        varModifier = "[mut] ";
        advance();
    }

    shared_ptr<ASTNode> varNode = nullptr;
    if (peek().type == TokenType::T_ID) {
        varNode = make_shared<ASTNode>("Variable", varModifier + advance().lexeme);
    } else {
        error("Esperado nome da variavel apos 'let'");
    }

    if (peek().type == TokenType::T_COLON) {
        advance();
        if (peek().type == TokenType::T_TYPE) {
            varNode->value += " : " + advance().lexeme;
        } else {
            error("Esperado tipo da variavel apos ':'");
        }
    }

    // Adiciona a Variável como o primeiro filho (ramo esquerdo)
    node->addChild(varNode);

    if (!match(TokenType::T_ASSIGN)) {
        error("Esperado '=' na declaracao");
    }

    // Adiciona a Expressão de valor como o segundo filho (ramo direito)
    node->addChild(parseExpression());

    if (!match(TokenType::T_SEMICOLON)) {
        error("Esperado ';' no final da declaracao");
    }
    return node;
}

shared_ptr<ASTNode> Parser::parseStatement() {
    TokenType type = peek().type;
    if (type == TokenType::T_LET) {
        return parseDeclaration();
    }
    else if (type == TokenType::T_PRINTLN) {
        return parsePrintStmt();
    }
    else if (type == TokenType::T_IF) {
        return parseIf();
    }
    else if (type == TokenType::T_WHILE) {
        return parseWhile();
    }
    else if (type == TokenType::T_ID) {
        return parseAssignment();
    }
    else {
        error("Comando invalido ou expressao fora de contexto");
        return nullptr;
    }
}

shared_ptr<ASTNode> Parser::parseAssignment() {
    auto node = make_shared<ASTNode>("Assignment");

    shared_ptr<ASTNode> varNode = nullptr;
    if (peek().type == TokenType::T_ID) {
        varNode = make_shared<ASTNode>("Variable", advance().lexeme);
    } else {
        error("Esperado nome da variavel para atribuicao");
    }

    // Variável vira o filho esquerdo
    node->addChild(varNode);

    if (!match(TokenType::T_ASSIGN)) {
        error("Esperado '=' apos o nome da variavel");
    }

    // Expressão de valor vira o filho direito
    node->addChild(parseExpression());

    if (!match(TokenType::T_SEMICOLON)) {
        error("Esperado ';' no final da atribuicao");
    }
    return node;
}

shared_ptr<ASTNode> Parser::parseIf() {
    auto node = make_shared<ASTNode>("IfStatement");
    match(TokenType::T_IF);

    node->addChild(parseExpression());

    if (!match(TokenType::T_LBRACE)) {
        error("Esperado '{' para abrir o bloco do 'if'");
    }

    auto thenBranch = make_shared<ASTNode>("ThenBlock");
    while (peek().type != TokenType::T_RBRACE && peek().type != TokenType::T_EOF) {
        thenBranch->addChild(parseStatement());
    }
    node->addChild(thenBranch);

    if (!match(TokenType::T_RBRACE)) {
        error("Esperado '}' para fechar o bloco do 'if'");
    }

    if (peek().type == TokenType::T_ELSE) {
        advance();
        if (!match(TokenType::T_LBRACE)) {
            error("Esperado '{' para abrir o bloco do 'else'");
        }

        auto elseBranch = make_shared<ASTNode>("ElseBlock");
        while (peek().type != TokenType::T_RBRACE && peek().type != TokenType::T_EOF) {
            elseBranch->addChild(parseStatement());
        }
        node->addChild(elseBranch);

        if (!match(TokenType::T_RBRACE)) {
            error("Esperado '}' para fechar o bloco do 'else'");
        }
    }
    return node;
}

shared_ptr<ASTNode> Parser::parseWhile() {
    auto node = make_shared<ASTNode>("WhileStatement");
    match(TokenType::T_WHILE);

    node->addChild(parseExpression());

    if (!match(TokenType::T_LBRACE)) {
        error("Esperado '{' para abrir o bloco do 'while'");
    }

    auto body = make_shared<ASTNode>("BodyBlock");
    while (peek().type != TokenType::T_RBRACE && peek().type != TokenType::T_EOF) {
        body->addChild(parseStatement());
    }
    node->addChild(body);

    if (!match(TokenType::T_RBRACE)) {
        error("Esperado '}' para fechar o bloco do 'while'");
    }
    return node;
}

shared_ptr<ASTNode> Parser::parsePrintStmt() {
    auto node = make_shared<ASTNode>("PrintStatement");
    match(TokenType::T_PRINTLN);

    if (!match(TokenType::T_BANG)) error("Esperado '!' apos 'println'");
    if (!match(TokenType::T_LPAREN)) error("Esperado '(' apos 'println!'");

    if (peek().type == TokenType::T_STRING) {
        node->value = advance().lexeme;
    } else {
        error("Esperado string literal no 'println!'");
    }

    if (peek().type == TokenType::T_COMMA) {
        advance();
        node->addChild(parseExpression());
    }

    if (!match(TokenType::T_RPAREN)) error("Esperado ')' para fechar 'println!'");
    if (!match(TokenType::T_SEMICOLON)) error("Esperado ';' no final");

    return node;
}
