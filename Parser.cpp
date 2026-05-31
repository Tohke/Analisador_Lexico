#include <iostream>
#include <vector>

/*
TODO: Falta implementar:
    - parseAssignment(): Validação de atribuição de variáveis já declaradas (ex: soma = 30;).
    - parsePrintStmt(): Adaptação da regra de impressão para o modelo do Rust (ex: validar a sintaxe exata de println!("{}", soma);).
    - parseIf(): Estrutura condicional. Diferente do MicroC original, em Rust não precisamos exigir parênteses ao redor da expressão, mas as chaves {} são estritamente obrigatórias.
    - parseWhile(): Laço de repetição, seguindo a mesma lógica de parênteses opcionais e chaves obrigatórias do if.
    - AST
    - Aprimoramento do Tratamento de Erros
    - Melhorias
 */





//Parser Descendente Recursivo -> cada regra da gramática se torna uma funcão no código
class Parser {
private:
    vector<Token> tokens; // Lista de tokens gerada pelo Scanner
    size_t pos;           // Controle de posição dos tokens
public:
    // Construtor recebe a lista de tokens do Scanner
    Parser(vector<Token> t) : tokens(t), pos(0) {}

    // ============================================== // ==============================================
    // Olha o token atual sem avançar
    Token peek() {
        if (pos >= tokens.size()) {
            return tokens.back(); // Retorna o EOF de segurança
        }
        return tokens[pos];
    }
    // ============================================== // ==============================================
    // Retorna o token atual e avança para o próximo
    Token advance() {
        if (pos < tokens.size()) {
            pos++;
        }
        return tokens[pos - 1];
    }
    // ============================================== // ==============================================
    // Verifica se o token atual é o esperado. Se for, consome e retorna true.
    bool match(TokenType expected) {
        if (peek().type == expected) {
            advance();
            return true;
        }
        return false;
    }
    // ============================================== // ==============================================
    // Tratamento de erros utilizando Panic Mode
    void error(string message) {
        Token t = peek();
        throw runtime_error(
            "Erro Sintatico: " + message +
            " (Token: " + t.lexeme + ") na linha " + to_string(t.line)
        );
    }

    // ============================================== // ==============================================
    void parseProgram() {
        // Regra: Program -> Statement*
        while (peek().type != TokenType::T_EOF) {
            parseStatement();
        }
    }
    // ============================================== // ==============================================
    void parseExpression() {
            // Toda expressão começa com pelo menos um Termo
            parseTerm();

            // Enquanto encontrar + ou -, continua consumindo novos termos
            while (peek().type == TokenType::T_PLUS || peek().type == TokenType::T_MINUS) {
                advance();   // Consome o operador (+ ou -)
                parseTerm(); // Avalia o próximo lado da operação
            }
        }
    // ============================================== // ==============================================
    void parseTerm() {
            // Todo termo começa com pelo menos um Fator
            parseFactor();

            // Enquanto encontrar * ou /, continua consumindo novos fatores
            while (peek().type == TokenType::T_MULT || peek().type == TokenType::T_DIV) {
                advance();     // Consome o operador (* ou /)
                parseFactor(); // Avalia o próximo lado da operação
            }
        }
    // ============================================== // ==============================================
    void parseFactor() {
            TokenType type = peek().type;

            // Possibilidades:
            // Ser um número
            if (type == TokenType::T_NUM) {
                advance(); // Consome o número
            }
            // Ser uma variável (identificador)
            else if (type == TokenType::T_ID) {
                advance(); // Consome a variável
            }
            // Ser uma nova expressão entre parênteses
            else if (type == TokenType::T_LPAREN) {
                advance(); // Consome o '('

                parseExpression(); // Volta recursivamente para avaliar o que está dentro

                if (!match(TokenType::T_RPAREN)) {
                    error("Esperado ')' apos expressao");
                }
            }
            else {
                error("Esperado numero, variavel ou '(' na expressao");
            }
        }
    // ============================================== // ==============================================
    void parseDeclaration() {
            match(TokenType::T_LET); // Consome o 'let'

            // Suporte a variáveis mutáveis (ex: let mut x = 10;) -> mut
            if (peek().type == TokenType::T_MUT) {
                advance(); // Consome o 'mut'
            }

            // Verifica se é um identificador válido
            if (!match(TokenType::T_ID)) {
                error("Esperado nome da variavel apos 'let'");
            }

            // Verifica se tem tipagem explícita (ex: let num1: i32 = 10;) -> i32, i64, ...
            if (peek().type == TokenType::T_COLON) {
                advance(); // Consome o ':'
                if (!match(TokenType::T_TYPE)) {
                    error("Esperado tipo da variavel (ex: i32) apos ':'");
                }
            }
// TODO: Talvez precise mudar algo para igualdade ==
            // Deve ser seguido por '='
            if (!match(TokenType::T_ASSIGN)) {
                error("Esperado '=' na declaracao");
            }
            // Chama o parse de expressões matemáticas (que vamos criar depois)
            parseExpression();

            // Toda declaração no final exige ponto e vírgula
            if (!match(TokenType::T_SEMICOLON)) {
                error("Esperado ';' no final da instrucao");
            }
        }
    // ============================================== // ==============================================
    // Funcão na qual decide qual regra gramatical vai utilizar ao olhar para o primeiro token da instrucão atual
    void parseStatement() {
        TokenType type = peek().type;

        // Verifica qual o token atual para decidir a regra de derivação
        if (type == TokenType::T_LET) { // Aqui por exemplo, seria INT em C
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
    // ============================================== // ==============================================
    void parseDeclaration() {}
    void parseAssignment() {}
    void parsePrintStmt() {}
    void parseIf() {}
    void parseWhile() {}

    // Funções de Expressão Matemática
    void parseExpression() {}
    void parseTerm() {}
    void parseFactor() {}
};
