#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

// Certifique-se de que o nome do arquivo bate exatamente com o seu arquivo do Scanner
#include "Analisador_lexico.h" 

using namespace std;

// =====================================================================
// ESTRUTURA DA ÁRVORE SINTÁTICA ABSTRATA (AST)
// =====================================================================
struct ASTNode {
    string type;                     
    string value;                    
    vector<shared_ptr<ASTNode>> children;

    ASTNode(string t, string v = "") : type(t), value(v) {}

    void addChild(shared_ptr<ASTNode> child) {
        if (child) children.push_back(child);
    }
};

// Imprime a árvore utilizando os caracteres solicitados mantendo o alinhamento
void printAST(const shared_ptr<ASTNode>& node, string indent = "", bool isLast = true) {
    if (!node) return;

    cout << indent;
    
    if (!indent.empty()) {
        if (isLast) {
            cout << "|__ ";
        } else {
            cout << "|-- ";
        }
    } else {
        cout << "- "; 
    }

    cout << node->type;
    if (!node->value.empty()) {
        cout << " (" << node->value << ")";
    }
    cout << "\n";

    string newIndent = indent;
    if (!indent.empty()) {
        newIndent += isLast ? "    " : "|   ";
    } else {
        newIndent += "  "; 
    }

    for (size_t i = 0; i < node->children.size(); ++i) {
        printAST(node->children[i], newIndent, i == node->children.size() - 1);
    }
}

// =====================================================================
// CLASSE PARSER (RETORNANDO NÓS DA AST)
// =====================================================================
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
            " (Token: '" + t.lexeme + "') na linha " + to_string(t.line)
        );
    }

    shared_ptr<ASTNode> parseProgram() {
        auto node = make_shared<ASTNode>("Program");
        while (peek().type != TokenType::T_EOF) {
            node->addChild(parseStatement());
        }
        return node;
    }

    shared_ptr<ASTNode> parseAdditive() {
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

    shared_ptr<ASTNode> parseExpression() {
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

    shared_ptr<ASTNode> parseTerm() {
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

    shared_ptr<ASTNode> parseFactor() {
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

    shared_ptr<ASTNode> parseDeclaration() {
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

    shared_ptr<ASTNode> parseStatement() {
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

    shared_ptr<ASTNode> parseAssignment() {
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

    shared_ptr<ASTNode> parseIf() {
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

    shared_ptr<ASTNode> parseWhile() {
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

    shared_ptr<ASTNode> parsePrintStmt() {
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
};

// =====================================================================
// FUNÇÃO PRINCIPAL (MAIN)
// =====================================================================
int main() {
    string code = R"(
    let num1: i32 = 10;     
    let num2 = 20;          
    let soma = num1 + num2;

    if soma >= 30 {
        println!("{}", soma);
    }
    )";

    Scanner scanner(code);
    vector<Token> tokens;

    try {
        cout << "=========================================" << endl;
        cout << "       FASE 1: ANALISE LEXICA            " << endl;
        cout << "=========================================" << endl;

        Token token = scanner.nextToken();
        while (token.type != TokenType::T_EOF) {
            cout << "--------------------------------------\n"
                 << "| Token  : " << tokenTypeToString(token.type) << '\n'
                 << "| Lexeme : " << token.lexeme << '\n'
                 << "| Linha  : " << token.line << '\n'
                 << "--------------------------------------\n\n";

            tokens.push_back(token);
            token = scanner.nextToken();
        }
        tokens.push_back(token); 

        cout << "Fim da analise lexica sem erros.\n" << endl;

        cout << "=========================================" << endl;
        cout << "  FASE 2: ARVORE SINTATICA ABSTRATA (AST)" << endl;
        cout << "=========================================" << endl;

        Parser parser(tokens);
        shared_ptr<ASTNode> astRoot = parser.parseProgram();

        printAST(astRoot);

        cout << "=========================================" << endl;
        cout << "Compilacao e Geracao da AST Concluidas!  " << endl;
        cout << "=========================================" << endl;

    }
    catch (exception& e) {
        cout << "\n[ERRO ENCONTRADO DURANTE A EXECUCAO]" << endl;
        cerr << e.what() << endl;
    }

    return 0;
}