#include <iostream>
#include <vector>
#include <string>

#include "Lexer.h"
#include "AST.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "SDT.hpp"

using namespace std;

// =====================================================================
// CÓDIGOS DE EXEMPLO (descomente o que quiser testar)
// =====================================================================
/*  Código teste - Correto
string code = R"(
    let num1: i32 = 10;
    let num2 = 20;
    let soma = num1 + num2;

    if soma >= 30 {
        println!("{}", soma);
    }
)";

====================================================
Código teste - Errado (Panic Mode)
string code = R"(
let num1: i32 = 10;
let num2 = ;        // Erro 1: Faltou o valor antes do ponto e vírgula
let soma = num1 + num2;
if soma >= 30         // Erro 2: Faltou a chave {
    println!("{}", soma);
}
)";
====================================================
SEMANTICA CERTA
====================================================
R"(
        let limite: i32 = 5;
        let mut contador: i32 = 0;
        let passo = 1;

        while contador < limite {
            // Atribuição válida, pois 'contador' foi declarado com 'mut'
            contador = contador + passo;
        }

        if contador == limite {
            let mensagem_sucesso = 1;
            println!("{}", contador);
        } else {
            let erro = 0;
            println!("{}", erro);
        }
    )"
====================================================
SEMANTICA ERRADA
====================================================
R"(
            let taxa: i32 = 10;
            let mut total = 0;

            // ERRO 1: Tentativa de reatribuir valor a uma variável imutável
            taxa = 20;

            // ERRO 2: Uso de variável que nunca foi declarada ('desconto')
            total = taxa - desconto;

            // ERRO 3: Redeclaração da mesma variável no mesmo escopo
            let total = 100;

            if total > 0 {
                println!("{}", total);
            }
        )"
*/
int main() {
    string code = R"(
        let limite: i32 = 5;
        let mut contador: i32 = 0;
        let passo = 1;

        while contador < limite {
            // Atribuição válida, pois 'contador' foi declarado com 'mut'
            contador = contador + passo;
        }

        if contador == limite {
            let mensagem_sucesso = 1;
            println!("{}", contador);
        } else {
            let erro = 0;
            println!("{}", erro);
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
        cout << "       FASE 3: ANALISE SEMANTICA         " << endl;
        cout << "=========================================" << endl;

        SemanticAnalyzer semanticAnalyzer;
        semanticAnalyzer.analyze(astRoot);

        cout << "=========================================" << endl;
        cout << "       FASE 4: TRADUCAO SDT (RPN)        " << endl;
        cout << "=========================================" << endl;

        PostfixTranslator postfixTranslator;
        postfixTranslator.generate(astRoot);
        postfixTranslator.printOutput();

        cout << "=========================================" << endl;
        cout << "       FASE 5: TRADUCAO SDT (TAC)        " << endl;
        cout << "=========================================" << endl;

        TACTranslator tacTranslator;
        tacTranslator.generate(astRoot);
        tacTranslator.printInstructions();

        cout << "=========================================" << endl;
        cout << "       FASE 6: PRETTY PRINTER            " << endl;
        cout << "=========================================" << endl;

        PrettyPrinter prettyPrinter;
        prettyPrinter.generate(astRoot);
        prettyPrinter.printSource();

        cout << "=========================================" << endl;
        cout << "Compilacao e Geracao da AST Concluidas!  " << endl;
        cout << "=========================================" << endl;

    }
    catch (exception& e) {
        cout << "\n[ERRO ENCONTRADO DURANTE A EXECUCAO]" << endl;
        cerr << e.what() << endl;
    }

    cout << "=========================================" << endl;
    cout << "Compilacao Concluida!                    " << endl;
    cout << "=========================================" << endl;
    return 0;
}
