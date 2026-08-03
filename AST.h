#ifndef AST_H
#define AST_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>

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
inline void printAST(const shared_ptr<ASTNode>& node, string indent = "", bool isLast = true) {
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

#endif
