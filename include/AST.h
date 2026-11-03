#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

// =====================================================================
// ESTRUTURA DA ÁRVORE SINTÁTICA ABSTRATA (AST)
// =====================================================================
struct ASTNode {
    std::string type;
    std::string value;
    std::vector<std::shared_ptr<ASTNode>> children;

    explicit ASTNode(std::string t, std::string v = "") : type(std::move(t)), value(std::move(v)) {}

    void addChild(std::shared_ptr<ASTNode> child) {
        if (child) children.push_back(std::move(child));
    }
};

// Imprime a árvore utilizando os caracteres solicitados mantendo o alinhamento
void printAST(const std::shared_ptr<ASTNode>& node, std::string indent = "", bool isLast = true);

#endif // AST_H
