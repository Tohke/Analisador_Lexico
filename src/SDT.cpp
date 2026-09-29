#include "SDT.hpp"
#include <iostream>

using namespace std;

// =====================================================================
// PostfixTranslator (Notação Polonesa Reversa / RPN)
// =====================================================================
void PostfixTranslator::emit(const string& token) {
    output.push_back(token);
    actionLog.push_back("emit(" + token + ")");
}

vector<string> PostfixTranslator::generate(shared_ptr<ASTNode> node) {
    output.clear();
    actionLog.clear();
    visit(node);
    return output;
}

void PostfixTranslator::printOutput() const {
    cout << "Saida RPN: ";
    for (const auto& token : output) {
        cout << token << " ";
    }
    cout << "\n";
}

void PostfixTranslator::visitProgram(shared_ptr<ASTNode> node) {
    for (auto child : node->children) visit(child);
}

void PostfixTranslator::visitLetAssignment(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 2) {
        visit(node->children[1]); // Expr
        // O RPN ignora declarações fora de expressoes, ou pode emitir =
        // O PDF foca em expressoes. Vamos ignorar ou omitir? 
        // O PDF diz: "Percurso estritamente pós-ordem na subárvore de expressões."
        // Entao omitimos atribuições no fluxo principal para nao poluir, ou emitimos.
        // Iremos omitir, já que RPN é para expressões.
    }
}

void PostfixTranslator::visitAssignment(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 2) {
        visit(node->children[1]); // Expr
    }
}

void PostfixTranslator::visitPrintStatement(shared_ptr<ASTNode> node) {
    for (auto child : node->children) visit(child);
}

void PostfixTranslator::visitIfStatement(shared_ptr<ASTNode> node) {
    for (auto child : node->children) visit(child);
}

void PostfixTranslator::visitWhileStatement(shared_ptr<ASTNode> node) {
    for (auto child : node->children) visit(child);
}

void PostfixTranslator::visitForStatement(shared_ptr<ASTNode> node) {
    for (auto child : node->children) visit(child);
}

void PostfixTranslator::visitOp(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 1) visit(node->children[0]);
    if (node->children.size() >= 2) visit(node->children[1]);
    string op = node->type.substr(4); // "Op: +" -> "+"
    emit(op);
}

void PostfixTranslator::visitCondition(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 1) visit(node->children[0]);
    if (node->children.size() >= 2) visit(node->children[1]);
    string op = node->type.substr(11); // "Condition: <" -> "<"
    emit(op);
}

void PostfixTranslator::visitLiteralNumber(shared_ptr<ASTNode> node) {
    emit(node->value);
}

void PostfixTranslator::visitVariable(shared_ptr<ASTNode> node) {
    string varName = node->value;
    size_t colonPos = varName.find(" : ");
    if (colonPos != string::npos) varName = varName.substr(0, colonPos);
    if (varName.find("[mut] ") == 0) varName = varName.substr(6);
    emit(varName);
}

// =====================================================================
// TACTranslator (Gerador de Código de Três Endereços)
// =====================================================================
string TACTranslator::newTemp() {
    tempCount++;
    return "t" + to_string(tempCount);
}

string TACTranslator::newLabel() {
    labelCount++;
    return "L" + to_string(labelCount);
}

void TACTranslator::emit(const string& instruction, const string& ruleNote) {
    instructions.push_back(instruction);
    if (!ruleNote.empty()) {
        actionLog.push_back(ruleNote);
    }
}

string TACTranslator::extractVarName(const string& rawValue) {
    string varName = rawValue;
    size_t colonPos = varName.find(" : ");
    if (colonPos != string::npos) varName = varName.substr(0, colonPos);
    if (varName.find("[mut] ") == 0) varName = varName.substr(6);
    return varName;
}

vector<string> TACTranslator::generate(shared_ptr<ASTNode> node) {
    instructions.clear();
    actionLog.clear();
    tempCount = 0;
    labelCount = 0;
    visit(node);
    return instructions;
}

void TACTranslator::printInstructions() const {
    cout << "Saida TAC:\n";
    for (const auto& inst : instructions) {
        cout << inst << "\n";
    }
}

void TACTranslator::visitProgram(shared_ptr<ASTNode> node) {
    for (auto child : node->children) visit(child);
}

void TACTranslator::visitLetAssignment(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 2) {
        string varName = extractVarName(node->children[0]->value);
        visit(node->children[1]);
        string exprPlace = lastPlace;
        emit(varName + " = " + exprPlace);
    }
}

void TACTranslator::visitAssignment(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 2) {
        string varName = extractVarName(node->children[0]->value);
        visit(node->children[1]);
        string exprPlace = lastPlace;
        emit(varName + " = " + exprPlace);
    }
}

void TACTranslator::visitPrintStatement(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 1) {
        visit(node->children[0]);
        emit("print " + lastPlace);
    } else {
        emit("print " + node->value);
    }
}

void TACTranslator::visitIfStatement(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 1) visit(node->children[0]);
    string condPlace = lastPlace;

    string l_else = newLabel();
    string l_end = newLabel();

    emit("ifFalse " + condPlace + " goto " + l_else);

    if (node->children.size() >= 2) {
        visit(node->children[1]); // ThenBlock
    }
    emit("goto " + l_end);
    emit(l_else + ":");

    if (node->children.size() >= 3) {
        visit(node->children[2]); // ElseBlock
    }
    emit(l_end + ":");
}

void TACTranslator::visitWhileStatement(shared_ptr<ASTNode> node) {
    string l_start = newLabel();
    string l_end = newLabel();

    emit(l_start + ":");

    if (node->children.size() >= 1) visit(node->children[0]);
    string condPlace = lastPlace;

    emit("ifFalse " + condPlace + " goto " + l_end);

    if (node->children.size() >= 2) visit(node->children[1]); // BodyBlock

    emit("goto " + l_start);
    emit(l_end + ":");
}

void TACTranslator::visitForStatement(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 1 && node->children[0]->type != "Empty") {
        visit(node->children[0]); // Init
    }

    string l_start = newLabel();
    string l_end = newLabel();
    
    emit(l_start + ":");

    if (node->children.size() >= 2 && node->children[1]->type != "Empty") {
        visit(node->children[1]); // Cond
        string condPlace = lastPlace;
        emit("ifFalse " + condPlace + " goto " + l_end);
    }

    if (node->children.size() >= 4) {
        visit(node->children[3]); // Body
    }

    if (node->children.size() >= 3 && node->children[2]->type != "Empty") {
        visit(node->children[2]); // Inc
    }

    emit("goto " + l_start);
    emit(l_end + ":");
}

void TACTranslator::visitOp(shared_ptr<ASTNode> node) {
    if (node->children.size() >= 1) visit(node->children[0]);
    string leftPlace = lastPlace;

    if (node->children.size() >= 2) visit(node->children[1]);
    string rightPlace = lastPlace;

    string op = node->type.substr(4); // "Op: +" -> "+"
    string temp = newTemp();
    emit(temp + " = " + leftPlace + " " + op + " " + rightPlace);
    lastPlace = temp;
}

void TACTranslator::visitCondition(shared_ptr<ASTNode> node) {
    string op = node->type.substr(11); // "Condition: <" -> "<"

    if (op == "&&") {
        if (node->children.size() >= 1) visit(node->children[0]);
        string leftPlace = lastPlace;

        string l_false = newLabel();
        string l_end = newLabel();
        string temp = newTemp();

        emit("ifFalse " + leftPlace + " goto " + l_false, "Short-Circuit AND");

        if (node->children.size() >= 2) visit(node->children[1]);
        string rightPlace = lastPlace;

        emit(temp + " = " + rightPlace);
        emit("goto " + l_end);

        emit(l_false + ":");
        emit(temp + " = 0");

        emit(l_end + ":");
        lastPlace = temp;
        return;
    }

    if (op == "||") {
        if (node->children.size() >= 1) visit(node->children[0]);
        string leftPlace = lastPlace;

        string l_eval_right = newLabel();
        string l_end = newLabel();
        string temp = newTemp();

        emit("ifFalse " + leftPlace + " goto " + l_eval_right, "Short-Circuit OR");
        emit(temp + " = 1");
        emit("goto " + l_end);

        emit(l_eval_right + ":");
        if (node->children.size() >= 2) visit(node->children[1]);
        string rightPlace = lastPlace;

        emit(temp + " = " + rightPlace);

        emit(l_end + ":");
        lastPlace = temp;
        return;
    }

    if (node->children.size() >= 1) visit(node->children[0]);
    string leftPlace = lastPlace;

    if (node->children.size() >= 2) visit(node->children[1]);
    string rightPlace = lastPlace;

    string temp = newTemp();
    emit(temp + " = " + leftPlace + " " + op + " " + rightPlace);
    lastPlace = temp;
}

void TACTranslator::visitLiteralNumber(shared_ptr<ASTNode> node) {
    lastPlace = node->value;
}

void TACTranslator::visitVariable(shared_ptr<ASTNode> node) {
    lastPlace = extractVarName(node->value);
}

// =====================================================================
// PrettyPrinter (Reimpressor Canônico e Linter)
// =====================================================================
void PrettyPrinter::printIndent() {
    for (int i = 0; i < indentLevel; ++i) {
        sourceCode += "    "; // 4 espaços
    }
}

string PrettyPrinter::extractVarName(const string& rawValue) {
    string varName = rawValue;
    size_t colonPos = varName.find(" : ");
    if (colonPos != string::npos) varName = varName.substr(0, colonPos);
    if (varName.find("[mut] ") == 0) varName = varName.substr(6);
    return varName;
}

string PrettyPrinter::generate(shared_ptr<ASTNode> node) {
    sourceCode = "";
    indentLevel = 0;
    visit(node);
    return sourceCode;
}

void PrettyPrinter::printSource() const {
    cout << "Saida PrettyPrinter:\n";
    cout << sourceCode << "\n";
}

void PrettyPrinter::visitProgram(shared_ptr<ASTNode> node) {
    for (auto child : node->children) {
        visit(child);
        sourceCode += "\n";
    }
}

void PrettyPrinter::visitLetAssignment(shared_ptr<ASTNode> node) {
    printIndent();
    sourceCode += "let ";
    if (node->children.size() >= 2) {
        string rawVar = node->children[0]->value;
        if (rawVar.find("[mut] ") == 0) {
            sourceCode += "mut ";
        }
        string varName = extractVarName(rawVar);
        sourceCode += varName;
        
        size_t colonPos = rawVar.find(" : ");
        if (colonPos != string::npos) {
            sourceCode += ": " + rawVar.substr(colonPos + 3);
        }
        
        sourceCode += " = ";
        visit(node->children[1]);
        sourceCode += ";";
    }
}

void PrettyPrinter::visitAssignment(shared_ptr<ASTNode> node) {
    printIndent();
    if (node->children.size() >= 2) {
        sourceCode += extractVarName(node->children[0]->value) + " = ";
        visit(node->children[1]);
        sourceCode += ";";
    }
}

void PrettyPrinter::visitPrintStatement(shared_ptr<ASTNode> node) {
    printIndent();
    sourceCode += "println!(" + node->value;
    if (node->children.size() >= 1) {
        sourceCode += ", ";
        visit(node->children[0]);
    }
    sourceCode += ");";
}

void PrettyPrinter::visitIfStatement(shared_ptr<ASTNode> node) {
    printIndent();
    sourceCode += "if ";
    if (node->children.size() >= 1) visit(node->children[0]);
    sourceCode += " {\n";

    if (node->children.size() >= 2) {
        indentLevel++;
        for (auto child : node->children[1]->children) {
            visit(child);
            sourceCode += "\n";
        }
        indentLevel--;
    }
    
    printIndent();
    sourceCode += "}";

    if (node->children.size() >= 3) {
        sourceCode += " else {\n";
        indentLevel++;
        for (auto child : node->children[2]->children) {
            visit(child);
            sourceCode += "\n";
        }
        indentLevel--;
        printIndent();
        sourceCode += "}";
    }
}

void PrettyPrinter::visitWhileStatement(shared_ptr<ASTNode> node) {
    printIndent();
    sourceCode += "while ";
    if (node->children.size() >= 1) visit(node->children[0]);
    sourceCode += " {\n";

    if (node->children.size() >= 2) {
        indentLevel++;
        for (auto child : node->children[1]->children) {
            visit(child);
            sourceCode += "\n";
        }
        indentLevel--;
    }
    
    printIndent();
    sourceCode += "}";
}

void PrettyPrinter::visitForStatement(shared_ptr<ASTNode> node) {
    printIndent();
    std::string oldCode = sourceCode;
    sourceCode = "";
    int oldIndent = indentLevel;
    indentLevel = 0;

    if (node->children.size() >= 1 && node->children[0]->type != "Empty") {
        visit(node->children[0]);
    } else {
        sourceCode += ";";
    }
    std::string initCode = sourceCode;

    sourceCode = "";
    if (node->children.size() >= 2 && node->children[1]->type != "Empty") {
        visit(node->children[1]);
    }
    std::string condCode = sourceCode;

    sourceCode = "";
    if (node->children.size() >= 3 && node->children[2]->type != "Empty") {
        visit(node->children[2]);
        if (!sourceCode.empty() && sourceCode.back() == ';') {
            sourceCode.pop_back();
        }
    }
    std::string incCode = sourceCode;

    indentLevel = oldIndent;
    sourceCode = oldCode;

    sourceCode += "for (" + initCode + " " + condCode + "; " + incCode + ") {\n";

    if (node->children.size() >= 4) {
        indentLevel++;
        for (auto child : node->children[3]->children) {
            visit(child);
            sourceCode += "\n";
        }
        indentLevel--;
    }

    printIndent();
    sourceCode += "}";
}

void PrettyPrinter::visitOp(shared_ptr<ASTNode> node) {
    sourceCode += "(";
    if (node->children.size() >= 1) visit(node->children[0]);
    string op = node->type.substr(4); // "Op: +" -> "+"
    sourceCode += " " + op + " ";
    if (node->children.size() >= 2) visit(node->children[1]);
    sourceCode += ")";
}

void PrettyPrinter::visitCondition(shared_ptr<ASTNode> node) {
    sourceCode += "(";
    if (node->children.size() >= 1) visit(node->children[0]);
    string op = node->type.substr(11); // "Condition: <" -> "<"
    sourceCode += " " + op + " ";
    if (node->children.size() >= 2) visit(node->children[1]);
    sourceCode += ")";
}

void PrettyPrinter::visitLiteralNumber(shared_ptr<ASTNode> node) {
    sourceCode += node->value;
}

void PrettyPrinter::visitVariable(shared_ptr<ASTNode> node) {
    sourceCode += extractVarName(node->value);
}
