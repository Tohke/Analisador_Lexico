#ifndef SDT_HPP
#define SDT_HPP

#include "AST.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>

// Visitor Pattern Base
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visitProgram(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitLetAssignment(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitAssignment(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitPrintStatement(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitIfStatement(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitWhileStatement(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitForStatement(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitOp(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitCondition(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitLiteralNumber(std::shared_ptr<ASTNode> node) = 0;
    virtual void visitVariable(std::shared_ptr<ASTNode> node) = 0;

    // Dispatcher
    void visit(std::shared_ptr<ASTNode> node) {
        if (!node) return;
        if (node->type == "Program") visitProgram(node);
        else if (node->type == "LetAssignment") visitLetAssignment(node);
        else if (node->type == "Assignment") visitAssignment(node);
        else if (node->type == "PrintStatement") visitPrintStatement(node);
        else if (node->type == "IfStatement") visitIfStatement(node);
        else if (node->type == "WhileStatement") visitWhileStatement(node);
        else if (node->type == "ForStatement") visitForStatement(node);
        else if (node->type.find("Op:") == 0) visitOp(node);
        else if (node->type.find("Condition:") == 0) visitCondition(node);
        else if (node->type == "LiteralNumber") visitLiteralNumber(node);
        else if (node->type == "Variable") visitVariable(node);
        else if (node->type == "ThenBlock" || node->type == "ElseBlock" || node->type == "BodyBlock") {
            for (auto child : node->children) visit(child);
        }
    }
};

// =====================================================================
// Tradutor 1: PostfixTranslator (Notação Polonesa Reversa / RPN)
// =====================================================================
class PostfixTranslator : public ASTVisitor {
private:
    std::vector<std::string> output;
    std::vector<std::string> actionLog;

    void emit(const std::string& token);

public:
    std::vector<std::string> generate(std::shared_ptr<ASTNode> node);
    void printOutput() const;

    void visitProgram(std::shared_ptr<ASTNode> node) override;
    void visitLetAssignment(std::shared_ptr<ASTNode> node) override;
    void visitAssignment(std::shared_ptr<ASTNode> node) override;
    void visitPrintStatement(std::shared_ptr<ASTNode> node) override;
    void visitIfStatement(std::shared_ptr<ASTNode> node) override;
    void visitWhileStatement(std::shared_ptr<ASTNode> node) override;
    void visitForStatement(std::shared_ptr<ASTNode> node) override;
    void visitOp(std::shared_ptr<ASTNode> node) override;
    void visitCondition(std::shared_ptr<ASTNode> node) override;
    void visitLiteralNumber(std::shared_ptr<ASTNode> node) override;
    void visitVariable(std::shared_ptr<ASTNode> node) override;
};

// =====================================================================
// Tradutor 2: TACTranslator (Gerador de Código de Três Endereços)
// =====================================================================
class TACTranslator : public ASTVisitor {
private:
    int tempCount = 0;
    int labelCount = 0;
    std::vector<int> freeTemps;
    std::string lastPlace;
    std::vector<std::string> instructions;
    std::vector<std::string> actionLog;

    std::string newTemp();
    void freeTemp(const std::string& tempName);
    std::string newLabel();
    void emit(const std::string& instruction, const std::string& ruleNote = "");

    std::string extractVarName(const std::string& rawValue);

public:
    std::vector<std::string> generate(std::shared_ptr<ASTNode> node);
    void printInstructions() const;

    void visitProgram(std::shared_ptr<ASTNode> node) override;
    void visitLetAssignment(std::shared_ptr<ASTNode> node) override;
    void visitAssignment(std::shared_ptr<ASTNode> node) override;
    void visitPrintStatement(std::shared_ptr<ASTNode> node) override;
    void visitIfStatement(std::shared_ptr<ASTNode> node) override;
    void visitWhileStatement(std::shared_ptr<ASTNode> node) override;
    void visitForStatement(std::shared_ptr<ASTNode> node) override;
    void visitOp(std::shared_ptr<ASTNode> node) override;
    void visitCondition(std::shared_ptr<ASTNode> node) override;
    void visitLiteralNumber(std::shared_ptr<ASTNode> node) override;
    void visitVariable(std::shared_ptr<ASTNode> node) override;
};

// =====================================================================
// Tradutor 3: PrettyPrinter (Reimpressor Canônico e Linter)
// =====================================================================
class PrettyPrinter : public ASTVisitor {
private:
    int indentLevel = 0;
    std::string sourceCode;

    void printIndent();
    std::string extractVarName(const std::string& rawValue);

public:
    std::string generate(std::shared_ptr<ASTNode> node);
    void printSource() const;

    void visitProgram(std::shared_ptr<ASTNode> node) override;
    void visitLetAssignment(std::shared_ptr<ASTNode> node) override;
    void visitAssignment(std::shared_ptr<ASTNode> node) override;
    void visitPrintStatement(std::shared_ptr<ASTNode> node) override;
    void visitIfStatement(std::shared_ptr<ASTNode> node) override;
    void visitWhileStatement(std::shared_ptr<ASTNode> node) override;
    void visitForStatement(std::shared_ptr<ASTNode> node) override;
    void visitOp(std::shared_ptr<ASTNode> node) override;
    void visitCondition(std::shared_ptr<ASTNode> node) override;
    void visitLiteralNumber(std::shared_ptr<ASTNode> node) override;
    void visitVariable(std::shared_ptr<ASTNode> node) override;
};

#endif // SDT_HPP
