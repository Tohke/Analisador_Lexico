#include "AST.h"

#include <iostream>

using namespace std;

void printAST(const shared_ptr<ASTNode>& node, string indent, bool isLast) {
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
