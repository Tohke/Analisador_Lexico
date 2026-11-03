#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include <unordered_map>

// =====================================================================
// INTERPRETADOR DE CÓDIGO INTERMEDIÁRIO (TAC VM)
// Executa instruções TAC (Three-Address Code) geradas pelo compilador.
// =====================================================================
class TACInterpreter {
private:
    std::vector<std::string> instructions;
    std::unordered_map<std::string, double> memory;       // Memória de variáveis e temporários
    std::unordered_map<std::string, size_t> labelIndex;   // Mapa de rótulos -> índice da instrução
    std::vector<std::string> outputLog;                    // Log de saídas do programa

    // Resolve um operando: se for literal numérico retorna seu valor,
    // senão busca na memória de variáveis.
    double resolveOperand(const std::string& operand) const;

    // Verifica se uma string é um literal numérico (inteiro ou float)
    bool isNumberLiteral(const std::string& s) const;

    // Avalia uma operação binária entre dois operandos
    double evalBinaryOp(double left, const std::string& op, double right) const;

    // Pré-processa instruções para mapear rótulos aos seus índices
    void buildLabelIndex();

public:
    // Executa as instruções TAC fornecidas
    void execute(const std::vector<std::string>& tacInstructions);

    // Retorna o log de saídas geradas por instruções 'print'
    const std::vector<std::string>& getOutputLog() const { return outputLog; }

    // Imprime o resultado da execução formatado
    void printExecutionResult() const;

    // Imprime o estado final da memória (variáveis e seus valores)
    void printMemoryState() const;
};

#endif // INTERPRETER_H

