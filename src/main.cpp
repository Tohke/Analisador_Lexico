#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "Lexer.h"
#include "AST.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "SDT.hpp"
#include "Optimizer.h"
#include "Interpreter.h"

using namespace std;

// Código padrão caso nenhum arquivo seja passado
const string DEFAULT_CODE = R"(
    let limite: i32 = 5;
    let mut contador: i32 = 0;
    let passo = 1;

    while contador < limite {
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

int main(int argc, char* argv[]) {
    string code;
    string filename = "";

    if (argc > 1) {
        filename = argv[1];
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "[ERRO] Nao foi possivel abrir o arquivo: " << filename << endl;
            return 1;
        }

        // Validação da extensão (.mr ou Micro Rust)
        if (filename.length() >= 3 && filename.substr(filename.length() - 3) != ".mr") {
            cout << "[AVISO] Arquivo '" << filename << "' nao possui extensao recomendada (.mr).\n";
        }

        stringstream buffer;
        buffer << file.rdbuf();
        code = buffer.str();
        cout << "Compilando arquivo: " << filename << "\n\n";
    } else {
        cout << "Nenhum arquivo .mr informado. Executando codigo padrao embutido...\n";
        cout << "Uso: ./bin/compiler <arquivo.mr>\n\n";
        code = DEFAULT_CODE;
    }

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

        if (parser.hadError()) {
            cerr << "\n[ERRO] Compilacao interrompida devido a erros sintaticos." << endl;
            return 1;
        }

        cout << "=========================================" << endl;
        cout << "       FASE 3: ANALISE SEMANTICA         " << endl;
        cout << "=========================================" << endl;

        SemanticAnalyzer semanticAnalyzer;
        if (!semanticAnalyzer.analyze(astRoot)) {
            cerr << "\n[ERRO] Compilacao interrompida devido a erros semanticos." << endl;
            return 1;
        }

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
        vector<string> tacInstructions = tacTranslator.generate(astRoot);
        tacTranslator.printInstructions();

        cout << "=========================================" << endl;
        cout << "  FASE 6: OTIMIZACAO DE CODIGO (TAC)     " << endl;
        cout << "=========================================" << endl;

        TACOptimizer optimizer;
        vector<string> optimizedTAC = optimizer.optimize(tacInstructions);
        optimizer.printOptimizationLog();
        optimizer.printOptimizedInstructions();
        optimizer.printComparison();

        cout << "=========================================" << endl;
        cout << "       FASE 7: PRETTY PRINTER            " << endl;
        cout << "=========================================" << endl;

        PrettyPrinter prettyPrinter;
        prettyPrinter.generate(astRoot);
        prettyPrinter.printSource();

        cout << "=========================================" << endl;
        cout << "  FASE 8: BACK-END / EXECUCAO (TAC VM)   " << endl;
        cout << "=========================================" << endl;

        TACInterpreter interpreter;
        interpreter.execute(optimizedTAC);
        interpreter.printExecutionResult();
        interpreter.printMemoryState();

        cout << "\n=========================================" << endl;
        cout << "Compilacao e Execucao Concluidas com Sucesso!" << endl;
        cout << "=========================================" << endl;

    }
    catch (const exception& e) {
        cerr << "\n[ERRO ENCONTRADO DURANTE O PROCESSO]" << endl;
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}
