#include "Interpreter.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <cctype>
#include <algorithm>

using namespace std;

// =====================================================================
// Utilitários internos
// =====================================================================
static string trimStr(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool TACInterpreter::isNumberLiteral(const string& s) const {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-') {
        if (s.size() == 1) return false;
        i = 1;
    }
    bool hasDigit = false;
    bool hasDot = false;
    for (; i < s.size(); ++i) {
        if (isdigit(static_cast<unsigned char>(s[i]))) {
            hasDigit = true;
        } else if (s[i] == '.') {
            if (hasDot) return false;
            hasDot = true;
        } else {
            return false;
        }
    }
    return hasDigit;
}

double TACInterpreter::resolveOperand(const string& operand) const {
    if (isNumberLiteral(operand)) {
        return stod(operand);
    }
    auto it = memory.find(operand);
    if (it != memory.end()) {
        return it->second;
    }
    throw runtime_error("Erro de Execucao: variavel '" + operand + "' nao inicializada.");
}

double TACInterpreter::evalBinaryOp(double left, const string& op, double right) const {
    if (op == "+")  return left + right;
    if (op == "-")  return left - right;
    if (op == "*")  return left * right;
    if (op == "/") {
        if (right == 0.0) {
            throw runtime_error("Erro de Execucao: divisao por zero.");
        }
        return left / right;
    }
    if (op == "<")  return (left <  right) ? 1.0 : 0.0;
    if (op == ">")  return (left >  right) ? 1.0 : 0.0;
    if (op == "<=") return (left <= right) ? 1.0 : 0.0;
    if (op == ">=") return (left >= right) ? 1.0 : 0.0;
    if (op == "==") return (left == right) ? 1.0 : 0.0;
    if (op == "!=") return (left != right) ? 1.0 : 0.0;
    if (op == "&&") return (left != 0.0 && right != 0.0) ? 1.0 : 0.0;
    if (op == "||") return (left != 0.0 || right != 0.0) ? 1.0 : 0.0;

    throw runtime_error("Erro de Execucao: operador desconhecido '" + op + "'.");
}

void TACInterpreter::buildLabelIndex() {
    labelIndex.clear();
    for (size_t i = 0; i < instructions.size(); ++i) {
        string line = trimStr(instructions[i]);
        if (!line.empty() && line.back() == ':' && line.find(' ') == string::npos) {
            string label = line.substr(0, line.size() - 1);
            labelIndex[label] = i;
        }
    }
}

// =====================================================================
// Execução Principal
// =====================================================================
void TACInterpreter::execute(const vector<string>& tacInstructions) {
    instructions = tacInstructions;
    memory.clear();
    outputLog.clear();
    buildLabelIndex();

    size_t pc = 0; // Program Counter
    const size_t maxSteps = 1000000; // Proteção contra loops infinitos
    size_t steps = 0;

    while (pc < instructions.size()) {
        if (++steps > maxSteps) {
            throw runtime_error("Erro de Execucao: limite de instrucoes excedido (possivel loop infinito).");
        }

        string line = trimStr(instructions[pc]);
        if (line.empty()) {
            pc++;
            continue;
        }

        // 1. Label: "L1:" -> apenas avança
        if (line.back() == ':' && line.find(' ') == string::npos) {
            pc++;
            continue;
        }

        // 2. Goto: "goto L1"
        if (line.rfind("goto ", 0) == 0) {
            string label = trimStr(line.substr(5));
            auto it = labelIndex.find(label);
            if (it == labelIndex.end()) {
                throw runtime_error("Erro de Execucao: rotulo '" + label + "' nao encontrado.");
            }
            pc = it->second;
            continue;
        }

        // 3. Condicional: "ifFalse <cond> goto <label>"
        if (line.rfind("ifFalse ", 0) == 0) {
            size_t gotoPos = line.find(" goto ");
            if (gotoPos != string::npos) {
                string cond = trimStr(line.substr(8, gotoPos - 8));
                string label = trimStr(line.substr(gotoPos + 6));
                double condVal = resolveOperand(cond);

                if (condVal == 0.0) {
                    // Condição falsa -> desvia
                    auto it = labelIndex.find(label);
                    if (it == labelIndex.end()) {
                        throw runtime_error("Erro de Execucao: rotulo '" + label + "' nao encontrado.");
                    }
                    pc = it->second;
                    continue;
                }
                // Condição verdadeira -> continua sequencial
                pc++;
                continue;
            }
        }

        // 4. Print: "print <arg>"
        if (line.rfind("print ", 0) == 0) {
            string arg = trimStr(line.substr(6));
            double val = resolveOperand(arg);

            // Formata a saída: inteiro se não tem parte fracionária
            string output;
            if (floor(val) == val && !isinf(val)) {
                output = to_string(static_cast<long long>(val));
            } else {
                ostringstream oss;
                oss << val;
                output = oss.str();
            }

            outputLog.push_back(output);
            pc++;
            continue;
        }

        // 5. Atribuição: "dest = ..."
        size_t eqPos = line.find(" = ");
        if (eqPos != string::npos) {
            string dest = trimStr(line.substr(0, eqPos));
            string rhs = trimStr(line.substr(eqPos + 3));

            // Tokeniza o lado direito
            istringstream iss(rhs);
            vector<string> parts;
            string token;
            while (iss >> token) {
                parts.push_back(token);
            }

            if (parts.size() == 3) {
                // Operação binária: dest = src1 op src2
                double left = resolveOperand(parts[0]);
                double right = resolveOperand(parts[2]);
                memory[dest] = evalBinaryOp(left, parts[1], right);
            } else if (parts.size() == 1) {
                // Cópia: dest = src
                memory[dest] = resolveOperand(parts[0]);
            } else {
                throw runtime_error("Erro de Execucao: instrucao malformada '" + line + "'.");
            }

            pc++;
            continue;
        }

        // Instrução não reconhecida
        throw runtime_error("Erro de Execucao: instrucao desconhecida '" + line + "'.");
    }
}

// =====================================================================
// Saída Formatada
// =====================================================================
void TACInterpreter::printExecutionResult() const {
    cout << "\nSaida do Programa:" << endl;
    cout << "-------------------------------------------------------------------------" << endl;
    if (outputLog.empty()) {
        cout << "  (Nenhuma saida gerada pelo programa)" << endl;
    } else {
        for (const auto& out : outputLog) {
            cout << "  > " << out << endl;
        }
    }
    cout << "-------------------------------------------------------------------------" << endl;
    cout << "Total de saidas: " << outputLog.size() << endl;
}

void TACInterpreter::printMemoryState() const {
    cout << "\nEstado Final da Memoria (Variaveis):" << endl;
    cout << "-------------------------------------------------------------------------" << endl;
    cout << "| " << left << setw(25) << "Variavel"
         << " | " << setw(20) << "Valor" << " |" << endl;
    cout << "-------------------------------------------------------------------------" << endl;

    // Separa temporários de variáveis do programa
    vector<pair<string, double>> vars;
    vector<pair<string, double>> temps;

    for (const auto& entry : memory) {
        if (entry.first.size() >= 2 && entry.first[0] == 't' &&
            isdigit(static_cast<unsigned char>(entry.first[1]))) {
            temps.push_back(entry);
        } else {
            vars.push_back(entry);
        }
    }

    // Ordena alfabeticamente
    sort(vars.begin(), vars.end());
    sort(temps.begin(), temps.end());

    for (const auto& v : vars) {
        string valStr;
        if (floor(v.second) == v.second && !isinf(v.second)) {
            valStr = to_string(static_cast<long long>(v.second));
        } else {
            ostringstream oss;
            oss << v.second;
            valStr = oss.str();
        }
        cout << "| " << left << setw(25) << v.first
             << " | " << setw(20) << valStr << " |" << endl;
    }

    if (!temps.empty()) {
        cout << "| " << left << setw(25) << "--- Temporarios ---"
             << " | " << setw(20) << "" << " |" << endl;
        for (const auto& t : temps) {
            string valStr;
            if (floor(t.second) == t.second && !isinf(t.second)) {
                valStr = to_string(static_cast<long long>(t.second));
            } else {
                ostringstream oss;
                oss << t.second;
                valStr = oss.str();
            }
            cout << "| " << left << setw(25) << t.first
                 << " | " << setw(20) << valStr << " |" << endl;
        }
    }

    cout << "-------------------------------------------------------------------------" << endl;
}

