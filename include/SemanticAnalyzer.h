#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "AST.h"

// =====================================================================
// TABELA DE SÍMBOLOS
// =====================================================================
struct Symbol {
    std::string name;
    std::string type;
    bool isMut;
};

class SymbolTable {
private:
    // A pilha de escopos. O índice 0 é o escopo global.
    std::vector<std::unordered_map<std::string, Symbol>> scopes;

public:
    SymbolTable();

    void enterScope();
    void exitScope();

    // Retorna true se a variável foi definida com sucesso, false se já existir no escopo atual
    bool define(const std::string& name, const std::string& type, bool isMut);

    // Busca a variável do escopo mais interno até o global
    Symbol* resolve(const std::string& name);
};

// =====================================================================
// ANALISADOR SEMÂNTICO (VISITOR NA AST)
// =====================================================================
class SemanticAnalyzer {
private:
    SymbolTable symTable;

    // Função auxiliar para decodificar a string bruta gerada no parser
    void parseVariableNode(const std::string& rawValue, std::string& outName, std::string& outType, bool& outIsMut);

    void visit(std::shared_ptr<ASTNode> node);

public:
    void analyze(std::shared_ptr<ASTNode> root);
};

#endif // SEMANTIC_ANALYZER_H
