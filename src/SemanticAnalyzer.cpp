#include "SemanticAnalyzer.h"

#include <iostream>
#include <stdexcept>

using namespace std;

// ---------------------------------------------------------------------
// SymbolTable
// ---------------------------------------------------------------------
SymbolTable::SymbolTable() {
    enterScope(); // Inicializa o escopo global automaticamente
}

void SymbolTable::enterScope() {
    scopes.push_back(unordered_map<string, Symbol>());
}

void SymbolTable::exitScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

bool SymbolTable::define(const string& name, const string& type, bool isMut) {
    if (scopes.empty()) return false;

    auto& currentScope = scopes.back();
    if (currentScope.find(name) != currentScope.end()) {
        return false;
    }

    currentScope[name] = Symbol{name, type, isMut};
    return true;
}

Symbol* SymbolTable::resolve(const string& name) {
    for (int i = scopes.size() - 1; i >= 0; --i) {
        if (scopes[i].find(name) != scopes[i].end()) {
            return &scopes[i][name];
        }
    }
    return nullptr; // Não encontrado em nenhum escopo
}

// ---------------------------------------------------------------------
// SemanticAnalyzer
// ---------------------------------------------------------------------
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

void SemanticAnalyzer::analyze(shared_ptr<ASTNode> root) {
    try {
        visit(root);
        cout << "Analise Semantica concluida sem erros." << endl;
    } catch (const runtime_error& e) {
        cerr << "[ERRO SEMANTICO] " << e.what() << endl;
    }
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

            // Visita a expressão do lado direito ANTES de registrar a variável
            visit(exprNode);

            if (!symTable.define(varName, varType, isMut)) {
                throw runtime_error("A variavel '" + varName + "' ja foi declarada neste escopo.");
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
                throw runtime_error("Tentativa de atribuicao na variavel nao declarada '" + varName + "'.");
            }
            if (!sym->isMut) {
                throw runtime_error("Atribuicao invalida: A variavel '" + varName + "' eh imutavel. Tente declara-la com 'let mut'.");
            }

            visit(exprNode);
        }
    }
    else if (node->type == "IfStatement" || node->type == "WhileStatement") {
        // A condição (filho 0) deve ser avaliada no escopo atual
        if (node->children.size() >= 1) visit(node->children[0]);

        // Não abrimos o escopo aqui, delegamos para os blocos (ThenBlock, ElseBlock, BodyBlock)
        for (size_t i = 1; i < node->children.size(); ++i) {
            visit(node->children[i]);
        }
    }
    else if (node->type == "ThenBlock" || node->type == "ElseBlock" || node->type == "BodyBlock") {
        // Blocos de execução abrem um novo escopo
        symTable.enterScope();
        for (auto child : node->children) visit(child);
        symTable.exitScope();
    }
    else if (node->type == "PrintStatement") {
        for (auto child : node->children) visit(child);
    }
    else if (node->type.find("Op:") == 0 || node->type.find("Condition:") == 0) {
        for (auto child : node->children) visit(child);
    }
    else if (node->type == "Variable") {
        // Ao encontrar o uso de uma variável (ex: 'num1' + 'num2'), verificar se ela existe
        string varName = node->value;
        Symbol* sym = symTable.resolve(varName);
        if (!sym) {
            throw runtime_error("Uso de variavel nao declarada: '" + varName + "'.");
        }
    }
}
