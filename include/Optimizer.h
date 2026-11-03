#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <string>
#include <vector>

// Representa um registro de transformação aplicada pelo otimizador
struct OptimizationRecord {
    int pass;
    std::string technique;
    std::string before;
    std::string after;
};

// =====================================================================
// OTIMIZADOR DE CÓDIGO INTERMEDIÁRIO (TAC)
// =====================================================================
class TACOptimizer {
private:
    std::vector<std::string> originalInstructions;
    std::vector<std::string> optimizedInstructions;
    std::vector<OptimizationRecord> history;

    void logOptimization(int pass, const std::string& technique,
                         const std::string& before, const std::string& after);

    // Passes de otimização sobre a representação intermediária (TAC)
    bool passConstantFolding(std::vector<std::string>& insts, int pass);
    bool passAlgebraicSimplification(std::vector<std::string>& insts, int pass);
    bool passConstantAndCopyPropagation(std::vector<std::string>& insts, int pass);
    bool passCommonSubexpressionElimination(std::vector<std::string>& insts, int pass);
    bool passRedundantInstructions(std::vector<std::string>& insts, int pass);
    bool passDeadCodeElimination(std::vector<std::string>& insts, int pass);

public:
    std::vector<std::string> optimize(const std::vector<std::string>& instructions);

    const std::vector<OptimizationRecord>& getHistory() const { return history; }

    void printOptimizationLog() const;
    void printOptimizedInstructions() const;
    void printComparison() const;
};

#endif // OPTIMIZER_H
