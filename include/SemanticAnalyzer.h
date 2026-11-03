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
    int scopeLevel;
    std::string scopeName;
};

class SymbolTable {
private:
    // A pilha de escopos. O índice 0 é o escopo global.
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    std::vector<std::string> scopeNames;
    // Histórico de todos os símbolos declarados para exibição da tabela completa
    std::vector<Symbol> history;

public:
    SymbolTable();

    void enterScope(const std::string& scopeName = "Local");
    void exitScope();

    // Retorna true se a variável foi definida com sucesso, false se já existir no escopo atual
    bool define(const std::string& name, const std::string& type, bool isMut);

    // Busca a variável do escopo mais interno até o global
    Symbol* resolve(const std::string& name);

    // Exibe a tabela de símbolos formatada
    void printTable() const;
};

// =====================================================================
// ANALISADOR SEMÂNTICO (VISITOR NA AST)
// =====================================================================
class SemanticAnalyzer {
private:
    SymbolTable symTable;
    bool hasErrors = false;

    void reportError(const std::string& message);

    // Função auxiliar para decodificar a string bruta gerada no parser
    void parseVariableNode(const std::string& rawValue, std::string& outName, std::string& outType, bool& outIsMut);

    // Avalia e retorna o tipo de uma expressão, verificando compatibilidade de tipos
    std::string evaluateExpr(std::shared_ptr<ASTNode> node);

    // Verifica se o tipo da expressão é compatível com o tipo esperado
    bool areTypesCompatible(const std::string& expected, const std::string& actual, std::shared_ptr<ASTNode> exprNode) const;

    void visit(std::shared_ptr<ASTNode> node);

public:
    bool analyze(std::shared_ptr<ASTNode> root);
    bool hadError() const { return hasErrors; }
    void printSymbolTable() const { symTable.printTable(); }
};

#endif // SEMANTIC_ANALYZER_H
