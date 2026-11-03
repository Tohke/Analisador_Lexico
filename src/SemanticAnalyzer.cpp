#include "SemanticAnalyzer.h"

#include <iostream>
#include <iomanip>
#include <stdexcept>

using namespace std;

// ---------------------------------------------------------------------
// SymbolTable
// ---------------------------------------------------------------------
SymbolTable::SymbolTable() {
    enterScope("Global"); // Inicializa o escopo global automaticamente
}

void SymbolTable::enterScope(const string& scopeName) {
    scopes.push_back(unordered_map<string, Symbol>());
    scopeNames.push_back(scopeName);
}

void SymbolTable::exitScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
        scopeNames.pop_back();
    }
}

bool SymbolTable::define(const string& name, const string& type, bool isMut) {
    if (scopes.empty()) return false;

    auto& currentScope = scopes.back();
    if (currentScope.find(name) != currentScope.end()) {
        return false;
    }

    int level = static_cast<int>(scopes.size()) - 1;
    string sName = scopeNames.back() + " (nivel " + to_string(level) + ")";
    Symbol sym{name, type, isMut, level, sName};
    currentScope[name] = sym;
    history.push_back(sym);
    return true;
}

Symbol* SymbolTable::resolve(const string& name) {
    for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
        if (scopes[i].find(name) != scopes[i].end()) {
            return &scopes[i][name];
        }
    }
    return nullptr; // Não encontrado em nenhum escopo
}

void SymbolTable::printTable() const {
    cout << "\nTabela de Simbolos Registrados:\n";
    cout << "-------------------------------------------------------------------------\n";
    cout << "| " << left << setw(20) << "Identificador"
         << " | " << setw(10) << "Tipo"
         << " | " << setw(8) << "Mutavel"
         << " | " << setw(22) << "Escopo" << " |\n";
    cout << "-------------------------------------------------------------------------\n";

    if (history.empty()) {
        cout << "| (Nenhum simbolo registrado)                                           |\n";
    } else {
        for (const auto& sym : history) {
            cout << "| " << left << setw(20) << sym.name
                 << " | " << setw(10) << sym.type
                 << " | " << setw(8) << (sym.isMut ? "Sim" : "Nao")
                 << " | " << setw(22) << sym.scopeName << " |\n";
        }
    }
    cout << "-------------------------------------------------------------------------\n\n";
}

// ---------------------------------------------------------------------
// SemanticAnalyzer
// ---------------------------------------------------------------------
void SemanticAnalyzer::reportError(const string& message) {
    cerr << "[ERRO SEMANTICO] " << message << endl;
    hasErrors = true;
}

void SemanticAnalyzer::parseVariableNode(const string& rawValue, string& outName, string& outType, bool& outIsMut) {
    string val = rawValue;
    outIsMut = false;
    outType = "inferido"; // Tipo default caso não haja anotação de tipo

    // 1. Checa a flag de mutabilidade explícita
    if (val.find("[mut] ") == 0) {
        outIsMut = true;
        val = val.substr(6);
    }

    // 2. Checa se existe uma declaração de tipo explícita
    size_t colonPos = val.find(" : ");
    if (colonPos != string::npos) {
        outName = val.substr(0, colonPos);
        outType = val.substr(colonPos + 3);
    } else {
        outName = val;
    }
}

bool SemanticAnalyzer::areTypesCompatible(const string& expected, const string& actual, shared_ptr<ASTNode> exprNode) const {
    if (expected == "inferido" || expected == "erro" || actual == "erro") {
        return true;
    }
    if (expected == actual) {
        return true;
    }
    // Permite literal inteiro (inferido inicialmente como i32) ser atribuído a u32
    if (expected == "u32" && actual == "i32" && exprNode && exprNode->type == "LiteralNumber") {
        return true;
    }
    // Permite literais 0 ou 1 para variáveis do tipo bool (já que a linguagem usa 0 e 1)
    if (expected == "bool" && actual == "i32" && exprNode && exprNode->type == "LiteralNumber" &&
        (exprNode->value == "0" || exprNode->value == "1")) {
        return true;
    }
    return false;
}

string SemanticAnalyzer::evaluateExpr(shared_ptr<ASTNode> node) {
    if (!node || node->type == "Empty") return "vazio";

    if (node->type == "LiteralNumber") {
        if (node->value.find('.') != string::npos) {
            return "f32";
        }
        return "i32";
    }

    if (node->type == "Variable") {
        string varName = node->value;
        Symbol* sym = symTable.resolve(varName);
        if (!sym) {
            reportError("Uso de variavel nao declarada: '" + varName + "'.");
            return "erro";
        }
        return sym->type;
    }

    if (node->type.find("Op:") == 0) {
        string op = node->type.substr(4);
        string leftType = (node->children.size() >= 1) ? evaluateExpr(node->children[0]) : "erro";
        string rightType = (node->children.size() >= 2) ? evaluateExpr(node->children[1]) : "erro";

        if (leftType == "erro" || rightType == "erro") {
            return "erro";
        }

        if (leftType == rightType) {
            return leftType;
        }
        if ((leftType == "u32" && rightType == "i32" && node->children[1]->type == "LiteralNumber") ||
            (rightType == "u32" && leftType == "i32" && node->children[0]->type == "LiteralNumber")) {
            return "u32";
        }

        reportError("Incompatibilidade de tipos na operacao '" + op + "': nao eh possivel operar '" +
                    leftType + "' com '" + rightType + "'.");
        return "erro";
    }

    if (node->type.find("Condition:") == 0) {
        string op = node->type.substr(11);
        string leftType = (node->children.size() >= 1) ? evaluateExpr(node->children[0]) : "erro";
        string rightType = (node->children.size() >= 2) ? evaluateExpr(node->children[1]) : "erro";

        if (leftType == "erro" || rightType == "erro") {
            return "erro";
        }

        bool compatible = (leftType == rightType) ||
                          (leftType == "u32" && rightType == "i32" && node->children[1]->type == "LiteralNumber") ||
                          (rightType == "u32" && leftType == "i32" && node->children[0]->type == "LiteralNumber");

        if (!compatible) {
            reportError("Incompatibilidade de tipos na condicao '" + op + "': nao eh possivel comparar '" +
                        leftType + "' com '" + rightType + "'.");
            return "erro";
        }

        return "bool";
    }

    return "erro";
}

bool SemanticAnalyzer::analyze(shared_ptr<ASTNode> root) {
    hasErrors = false;
    visit(root);
    symTable.printTable();

    if (!hasErrors) {
        cout << "Analise Semantica concluida sem erros." << endl;
    } else {
        cout << "[Aviso] Analise Semantica finalizada com erros." << endl;
    }
    return !hasErrors;
}

void SemanticAnalyzer::visit(shared_ptr<ASTNode> node) {
    if (!node) return;

    // Delega o processamento baseado no tipo do nó da AST
    if (node->type == "Program") {
        for (auto child : node->children) visit(child);
    }
    else if (node->type == "LetAssignment") {
        if (node->children.size() >= 2) {
            auto varNode = node->children[0];
            auto exprNode = node->children[1];

            string varName, varType;
            bool isMut;
            parseVariableNode(varNode->value, varName, varType, isMut);

            // Avalia a expressão do lado direito ANTES de registrar a variável
            string exprType = evaluateExpr(exprNode);

            if (varType == "inferido") {
                if (exprType != "erro" && exprType != "vazio") {
                    varType = exprType;
                }
            } else if (!areTypesCompatible(varType, exprType, exprNode)) {
                reportError("Tipo incompativel na declaracao de '" + varName +
                            "': esperado '" + varType + "', mas foi atribuido '" + exprType + "'.");
            }

            if (!symTable.define(varName, varType, isMut)) {
                reportError("A variavel '" + varName + "' ja foi declarada neste escopo.");
            }
        }
    }
    else if (node->type == "Assignment") {
        if (node->children.size() >= 2) {
            auto varNode = node->children[0];
            auto exprNode = node->children[1];
            string varName = varNode->value;

            Symbol* sym = symTable.resolve(varName);
            if (!sym) {
                reportError("Tentativa de atribuicao na variavel nao declarada '" + varName + "'.");
            } else if (!sym->isMut) {
                reportError("Atribuicao invalida: A variavel '" + varName + "' eh imutavel. Tente declara-la com 'let mut'.");
            }

            string exprType = evaluateExpr(exprNode);
            if (sym && !areTypesCompatible(sym->type, exprType, exprNode)) {
                reportError("Tipo incompativel na atribuicao de '" + varName +
                            "': variavel eh do tipo '" + sym->type + "', mas recebeu '" + exprType + "'.");
            }
        }
    }
    else if (node->type == "IfStatement") {
        // A condição (filho 0) deve ser avaliada no escopo atual
        if (node->children.size() >= 1) evaluateExpr(node->children[0]);

        // Delegamos para os blocos (ThenBlock, ElseBlock)
        for (size_t i = 1; i < node->children.size(); ++i) {
            visit(node->children[i]);
        }
    }
    else if (node->type == "WhileStatement") {
        // A condição (filho 0) deve ser avaliada no escopo atual
        if (node->children.size() >= 1) evaluateExpr(node->children[0]);

        for (size_t i = 1; i < node->children.size(); ++i) {
            visit(node->children[i]);
        }
    }
    else if (node->type == "ForStatement") {
        // O cabeçalho do 'for' abre um escopo próprio para a variável de inicialização
        symTable.enterScope("For");

        // Filho 0: inicialização (LetAssignment, Assignment ou Empty)
        if (node->children.size() >= 1 && node->children[0]->type != "Empty") {
            visit(node->children[0]);
        }
        // Filho 1: condição
        if (node->children.size() >= 2 && node->children[1]->type != "Empty") {
            evaluateExpr(node->children[1]);
        }
        // Filho 2: incremento
        if (node->children.size() >= 3 && node->children[2]->type != "Empty") {
            visit(node->children[2]);
        }
        // Filho 3: BodyBlock
        if (node->children.size() >= 4) {
            visit(node->children[3]);
        }

        symTable.exitScope();
    }
    else if (node->type == "ThenBlock" || node->type == "ElseBlock" || node->type == "BodyBlock") {
        // Blocos de execução abrem um novo escopo
        symTable.enterScope(node->type);
        for (auto child : node->children) visit(child);
        symTable.exitScope();
    }
    else if (node->type == "PrintStatement") {
        for (auto child : node->children) evaluateExpr(child);
    }
    else if (node->type.find("Op:") == 0 || node->type.find("Condition:") == 0 ||
             node->type == "Variable" || node->type == "LiteralNumber") {
        evaluateExpr(node);
    }
}
