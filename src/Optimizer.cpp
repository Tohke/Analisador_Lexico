#include "Optimizer.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <cctype>
#include <algorithm>

using namespace std;

namespace {

enum class InstKind {
    LABEL,          // L1:
    GOTO,           // goto L1
    IF_FALSE,       // ifFalse x goto L1
    PRINT,          // print x
    ASSIGN_BINOP,   // dest = src1 op src2
    ASSIGN_COPY,    // dest = src1
    OTHER
};

struct ParsedInst {
    InstKind kind = InstKind::OTHER;
    string dest;
    string src1;
    string op;
    string src2;
    string label;
    string raw;
};

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool isNumberLiteral(const string& s) {
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

bool isFloatLiteral(const string& s) {
    return isNumberLiteral(s) && s.find('.') != string::npos;
}

bool isTemporary(const string& s) {
    if (s.size() < 2 || s[0] != 't') return false;
    for (size_t i = 1; i < s.size(); ++i) {
        if (!isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

bool isIdentifier(const string& s) {
    if (s.empty()) return false;
    if (!(isalpha(static_cast<unsigned char>(s[0])) || s[0] == '_')) return false;
    for (size_t i = 1; i < s.size(); ++i) {
        if (!(isalnum(static_cast<unsigned char>(s[i])) || s[i] == '_')) return false;
    }
    return true;
}

string formatDouble(double val) {
    if (floor(val) == val) {
        return to_string(static_cast<long long>(val));
    }
    ostringstream oss;
    oss << val;
    return oss.str();
}

ParsedInst parseInstruction(const string& line) {
    ParsedInst inst;
    inst.raw = trim(line);
    if (inst.raw.empty()) return inst;

    // 1. Label: "L1:"
    if (inst.raw.back() == ':' && inst.raw.find(' ') == string::npos) {
        inst.kind = InstKind::LABEL;
        inst.label = inst.raw.substr(0, inst.raw.size() - 1);
        return inst;
    }

    // 2. Goto: "goto L1"
    if (inst.raw.rfind("goto ", 0) == 0) {
        inst.kind = InstKind::GOTO;
        inst.label = trim(inst.raw.substr(5));
        return inst;
    }

    // 3. Conditional: "ifFalse <cond> goto <label>"
    if (inst.raw.rfind("ifFalse ", 0) == 0) {
        size_t gotoPos = inst.raw.find(" goto ");
        if (gotoPos != string::npos) {
            inst.kind = InstKind::IF_FALSE;
            inst.src1 = trim(inst.raw.substr(8, gotoPos - 8));
            inst.label = trim(inst.raw.substr(gotoPos + 6));
            return inst;
        }
    }

    // 4. Print: "print <arg>"
    if (inst.raw.rfind("print ", 0) == 0) {
        inst.kind = InstKind::PRINT;
        inst.src1 = trim(inst.raw.substr(6));
        return inst;
    }

    // 5. Assignments: "<dest> = <rhs>"
    size_t eqPos = inst.raw.find(" = ");
    if (eqPos != string::npos) {
        inst.dest = trim(inst.raw.substr(0, eqPos));
        string rhs = trim(inst.raw.substr(eqPos + 3));

        // Tokenize rhs by space (only if not a string literal)
        if (!rhs.empty() && rhs.front() != '"') {
            istringstream iss(rhs);
            vector<string> parts;
            string token;
            while (iss >> token) {
                parts.push_back(token);
            }
            if (parts.size() == 3) {
                inst.kind = InstKind::ASSIGN_BINOP;
                inst.src1 = parts[0];
                inst.op = parts[1];
                inst.src2 = parts[2];
                return inst;
            } else if (parts.size() == 1) {
                inst.kind = InstKind::ASSIGN_COPY;
                inst.src1 = parts[0];
                return inst;
            }
        } else {
            inst.kind = InstKind::ASSIGN_COPY;
            inst.src1 = rhs;
            return inst;
        }
    }

    return inst;
}

string rebuildInstruction(const ParsedInst& inst) {
    switch (inst.kind) {
        case InstKind::LABEL:
            return inst.label + ":";
        case InstKind::GOTO:
            return "goto " + inst.label;
        case InstKind::IF_FALSE:
            return "ifFalse " + inst.src1 + " goto " + inst.label;
        case InstKind::PRINT:
            return "print " + inst.src1;
        case InstKind::ASSIGN_BINOP:
            return inst.dest + " = " + inst.src1 + " " + inst.op + " " + inst.src2;
        case InstKind::ASSIGN_COPY:
            return inst.dest + " = " + inst.src1;
        default:
            return inst.raw;
    }
}

bool isCommutative(const string& op) {
    return op == "+" || op == "*" || op == "==" || op == "!=";
}

} // namespace

void TACOptimizer::logOptimization(int pass, const string& technique,
                                   const string& before, const string& after) {
    history.push_back(OptimizationRecord{pass, technique, before, after});
}

// ---------------------------------------------------------------------
// 1. Avaliação de Constantes (Constant Folding)
// ---------------------------------------------------------------------
bool TACOptimizer::passConstantFolding(vector<string>& insts, int pass) {
    bool changed = false;

    for (size_t i = 0; i < insts.size(); ++i) {
        ParsedInst p = parseInstruction(insts[i]);
        if (p.kind == InstKind::ASSIGN_BINOP && isNumberLiteral(p.src1) && isNumberLiteral(p.src2)) {
            bool isFlt = isFloatLiteral(p.src1) || isFloatLiteral(p.src2);
            double v1 = stod(p.src1);
            double v2 = stod(p.src2);

            // Evita divisão por zero em tempo de compilação
            if (p.op == "/" && v2 == 0.0) continue;

            string folded;
            if (p.op == "+") folded = isFlt ? formatDouble(v1 + v2) : to_string(stoll(p.src1) + stoll(p.src2));
            else if (p.op == "-") folded = isFlt ? formatDouble(v1 - v2) : to_string(stoll(p.src1) - stoll(p.src2));
            else if (p.op == "*") folded = isFlt ? formatDouble(v1 * v2) : to_string(stoll(p.src1) * stoll(p.src2));
            else if (p.op == "/") folded = isFlt ? formatDouble(v1 / v2) : to_string(stoll(p.src1) / stoll(p.src2));
            else if (p.op == "<") folded = (v1 < v2) ? "1" : "0";
            else if (p.op == ">") folded = (v1 > v2) ? "1" : "0";
            else if (p.op == "<=") folded = (v1 <= v2) ? "1" : "0";
            else if (p.op == ">=") folded = (v1 >= v2) ? "1" : "0";
            else if (p.op == "==") folded = (v1 == v2) ? "1" : "0";
            else if (p.op == "!=") folded = (v1 != v2) ? "1" : "0";
            else continue;

            string before = insts[i];
            string after = p.dest + " = " + folded;
            insts[i] = after;
            logOptimization(pass, "Avaliacao de Constantes (Constant Folding)", before, after);
            changed = true;
        }
    }

    return changed;
}

// ---------------------------------------------------------------------
// 2. Simplificação de Expressões (Algebraic Simplification)
// ---------------------------------------------------------------------
bool TACOptimizer::passAlgebraicSimplification(vector<string>& insts, int pass) {
    bool changed = false;

    for (size_t i = 0; i < insts.size(); ++i) {
        ParsedInst p = parseInstruction(insts[i]);
        if (p.kind != InstKind::ASSIGN_BINOP) continue;

        string simplified;
        if (p.op == "+") {
            if (p.src2 == "0" || p.src2 == "0.0") simplified = p.src1;
            else if (p.src1 == "0" || p.src1 == "0.0") simplified = p.src2;
        } else if (p.op == "-") {
            if (p.src2 == "0" || p.src2 == "0.0") simplified = p.src1;
            else if (p.src1 == p.src2) simplified = "0";
        } else if (p.op == "*") {
            if (p.src2 == "1" || p.src2 == "1.0") simplified = p.src1;
            else if (p.src1 == "1" || p.src1 == "1.0") simplified = p.src2;
            else if (p.src1 == "0" || p.src2 == "0") simplified = "0";
        } else if (p.op == "/") {
            if (p.src2 == "1" || p.src2 == "1.0") simplified = p.src1;
        } else if (p.op == "==" || p.op == "<=" || p.op == ">=") {
            if (p.src1 == p.src2) simplified = "1";
        } else if (p.op == "!=" || p.op == "<" || p.op == ">") {
            if (p.src1 == p.src2) simplified = "0";
        }

        if (!simplified.empty()) {
            string before = insts[i];
            string after = p.dest + " = " + simplified;
            insts[i] = after;
            logOptimization(pass, "Simplificacao Algebrica", before, after);
            changed = true;
        }
    }

    return changed;
}

// ---------------------------------------------------------------------
// 3. Propagação de Constantes e de Cópias (Constant & Copy Propagation)
// ---------------------------------------------------------------------
bool TACOptimizer::passConstantAndCopyPropagation(vector<string>& insts, int pass) {
    bool changed = false;

    // Conta quantas vezes cada variável é atribuída no programa inteiro
    unordered_map<string, int> defCount;
    for (const auto& line : insts) {
        ParsedInst p = parseInstruction(line);
        if (p.kind == InstKind::ASSIGN_COPY || p.kind == InstKind::ASSIGN_BINOP) {
            defCount[p.dest]++;
        }
    }

    // Constantes globais imutáveis: definidas uma única vez no bloco inicial (antes de qualquer desvio/rótulo)
    unordered_map<string, string> globalConsts;
    bool inEntryBlock = true;
    for (const auto& line : insts) {
        ParsedInst p = parseInstruction(line);
        if (p.kind == InstKind::LABEL || p.kind == InstKind::GOTO || p.kind == InstKind::IF_FALSE) {
            inEntryBlock = false;
            break;
        }
        if (inEntryBlock && p.kind == InstKind::ASSIGN_COPY &&
            !isTemporary(p.dest) && defCount[p.dest] == 1 && isNumberLiteral(p.src1)) {
            globalConsts[p.dest] = p.src1;
        }
    }

    // Mapas locais ao bloco básico atual
    unordered_map<string, string> localConsts;
    unordered_map<string, string> localCopies;

    auto resolveOperand = [&](const string& opnd, bool& wasConst) -> string {
        wasConst = false;
        if (localConsts.count(opnd)) {
            wasConst = true;
            return localConsts[opnd];
        }
        if (globalConsts.count(opnd)) {
            wasConst = true;
            return globalConsts[opnd];
        }
        if (localCopies.count(opnd)) {
            return localCopies[opnd];
        }
        return opnd;
    };

    auto invalidateVar = [&](const string& var) {
        localConsts.erase(var);
        localCopies.erase(var);
        // Remove entradas em localCopies cujo valor dependia de 'var'
        for (auto it = localCopies.begin(); it != localCopies.end(); ) {
            if (it->second == var) {
                it = localCopies.erase(it);
            } else {
                ++it;
            }
        }
    };

    for (size_t i = 0; i < insts.size(); ++i) {
        ParsedInst p = parseInstruction(insts[i]);

        // Fronteira de bloco básico limpa o estado local
        if (p.kind == InstKind::LABEL || p.kind == InstKind::GOTO) {
            localConsts.clear();
            localCopies.clear();
            continue;
        }

        if (p.kind == InstKind::IF_FALSE) {
            bool isConst = false;
            string newCond = resolveOperand(p.src1, isConst);
            if (newCond != p.src1) {
                string before = insts[i];
                p.src1 = newCond;
                insts[i] = rebuildInstruction(p);
                logOptimization(pass, isConst ? "Propagacao de Constantes" : "Propagacao de Copia",
                                before, insts[i]);
                changed = true;
            }
            localConsts.clear();
            localCopies.clear();
            continue;
        }

        if (p.kind == InstKind::PRINT) {
            bool isConst = false;
            string newVal = resolveOperand(p.src1, isConst);
            if (newVal != p.src1) {
                string before = insts[i];
                p.src1 = newVal;
                insts[i] = rebuildInstruction(p);
                logOptimization(pass, isConst ? "Propagacao de Constantes" : "Propagacao de Copia",
                                before, insts[i]);
                changed = true;
            }
            continue;
        }

        if (p.kind == InstKind::ASSIGN_BINOP) {
            bool isConst1 = false, isConst2 = false;
            string newSrc1 = resolveOperand(p.src1, isConst1);
            string newSrc2 = resolveOperand(p.src2, isConst2);

            if (newSrc1 != p.src1 || newSrc2 != p.src2) {
                string before = insts[i];
                p.src1 = newSrc1;
                p.src2 = newSrc2;
                insts[i] = rebuildInstruction(p);
                bool usedConst = (newSrc1 != p.src1 && isConst1) || (newSrc2 != p.src2 && isConst2) || isConst1 || isConst2;
                logOptimization(pass, usedConst ? "Propagacao de Constantes" : "Propagacao de Copia",
                                before, insts[i]);
                changed = true;
            }

            invalidateVar(p.dest);
            continue;
        }

        if (p.kind == InstKind::ASSIGN_COPY) {
            bool isConst = false;
            string newSrc = resolveOperand(p.src1, isConst);
            if (newSrc != p.src1) {
                string before = insts[i];
                p.src1 = newSrc;
                insts[i] = rebuildInstruction(p);
                logOptimization(pass, isConst ? "Propagacao de Constantes" : "Propagacao de Copia",
                                before, insts[i]);
                changed = true;
            }

            invalidateVar(p.dest);

            if (isNumberLiteral(p.src1)) {
                localConsts[p.dest] = p.src1;
            } else if (isIdentifier(p.src1) && !isTemporary(p.src1) && p.dest != p.src1) {
                localCopies[p.dest] = p.src1;
            }
        }
    }

    return changed;
}

// ---------------------------------------------------------------------
// 4. Eliminação de Subexpressões Comuns (Common Subexpression Elimination)
// ---------------------------------------------------------------------
bool TACOptimizer::passCommonSubexpressionElimination(vector<string>& insts, int pass) {
    bool changed = false;

    struct ExprEntry {
        string key;
        string holder;
        string src1;
        string src2;
    };

    vector<ExprEntry> availableExprs;

    auto invalidateVar = [&](const string& var) {
        availableExprs.erase(
            remove_if(availableExprs.begin(), availableExprs.end(),
                      [&](const ExprEntry& e) {
                          return e.holder == var || e.src1 == var || e.src2 == var;
                      }),
            availableExprs.end());
    };

    for (size_t i = 0; i < insts.size(); ++i) {
        ParsedInst p = parseInstruction(insts[i]);

        if (p.kind == InstKind::LABEL || p.kind == InstKind::GOTO || p.kind == InstKind::IF_FALSE) {
            availableExprs.clear();
            continue;
        }

        if (p.kind == InstKind::ASSIGN_COPY) {
            invalidateVar(p.dest);
            continue;
        }

        if (p.kind == InstKind::ASSIGN_BINOP) {
            string s1 = p.src1;
            string s2 = p.src2;
            if (isCommutative(p.op) && s2 < s1) {
                swap(s1, s2);
            }
            string key = p.op + "|" + s1 + "|" + s2;

            string foundHolder;
            for (const auto& entry : availableExprs) {
                if (entry.key == key) {
                    foundHolder = entry.holder;
                    break;
                }
            }

            if (!foundHolder.empty()) {
                string before = insts[i];
                string after = p.dest + " = " + foundHolder;
                insts[i] = after;
                logOptimization(pass, "Eliminacao de Subexpressoes Comuns (CSE)", before, after);
                changed = true;
                invalidateVar(p.dest);
            } else {
                invalidateVar(p.dest);
                // Se dest não foi um dos operandos modificados pela própria instrução, registra a expressão
                if (p.dest != p.src1 && p.dest != p.src2) {
                    availableExprs.push_back(ExprEntry{key, p.dest, p.src1, p.src2});
                }
            }
        }
    }

    return changed;
}

// ---------------------------------------------------------------------
// 5. Eliminação de Instruções Redundantes (Peephole / Saltos / Temporários)
// ---------------------------------------------------------------------
bool TACOptimizer::passRedundantInstructions(vector<string>& insts, int pass) {
    bool changed = false;

    for (size_t i = 0; i < insts.size(); ++i) {
        ParsedInst p = parseInstruction(insts[i]);

        // 5a. Colapso de temporário imediato:
        //   t1 = a op b   (ou t1 = a)
        //   x = t1
        // Vira:
        //   x = a op b    (desde que t1 não seja lido antes de ser redefinido)
        if ((p.kind == InstKind::ASSIGN_BINOP || p.kind == InstKind::ASSIGN_COPY) &&
            isTemporary(p.dest) && i + 1 < insts.size()) {
            ParsedInst nextP = parseInstruction(insts[i + 1]);
            if (nextP.kind == InstKind::ASSIGN_COPY && nextP.src1 == p.dest) {
                string tempName = p.dest;
                bool tempReadLater = false;
                for (size_t j = i + 2; j < insts.size(); ++j) {
                    ParsedInst later = parseInstruction(insts[j]);
                    if (later.src1 == tempName || later.src2 == tempName) {
                        tempReadLater = true;
                        break;
                    }
                    if ((later.kind == InstKind::ASSIGN_BINOP || later.kind == InstKind::ASSIGN_COPY) &&
                        later.dest == tempName) {
                        break; // Redefinido antes de ser lido
                    }
                }

                if (!tempReadLater) {
                    string before = insts[i] + " ; " + insts[i + 1];
                    p.dest = nextP.dest;
                    string after = rebuildInstruction(p);
                    insts[i] = after;
                    insts.erase(insts.begin() + static_cast<long>(i + 1));
                    logOptimization(pass, "Eliminacao de Temporario Redundante", before, after);
                    changed = true;
                    continue;
                }
            }
        }

        // 5b. Auto-atribuição redundante: x = x
        if (p.kind == InstKind::ASSIGN_COPY && p.dest == p.src1) {
            logOptimization(pass, "Eliminacao de Instrucao Redundante", insts[i], "(removida)");
            insts.erase(insts.begin() + static_cast<long>(i));
            --i;
            changed = true;
            continue;
        }

        // 5c. Desvio condicional com condição constante (ifFalse 1 goto L / ifFalse 0 goto L)
        if (p.kind == InstKind::IF_FALSE && isNumberLiteral(p.src1)) {
            double val = stod(p.src1);
            if (val != 0.0) {
                // Condição sempre verdadeira -> nunca desvia
                logOptimization(pass, "Simplificacao de Desvio Condicional", insts[i], "(removida: condicao sempre verdadeira)");
                insts.erase(insts.begin() + static_cast<long>(i));
                --i;
                changed = true;
                continue;
            } else {
                // Condição sempre falsa -> desvio incondicional
                string before = insts[i];
                string after = "goto " + p.label;
                insts[i] = after;
                logOptimization(pass, "Simplificacao de Desvio Condicional", before, after);
                changed = true;
                continue;
            }
        }

        // 5d. Salto redundante para o rótulo imediatamente seguinte: goto L1 seguido de L1:
        if (p.kind == InstKind::GOTO && i + 1 < insts.size()) {
            ParsedInst nextP = parseInstruction(insts[i + 1]);
            if (nextP.kind == InstKind::LABEL && nextP.label == p.label) {
                logOptimization(pass, "Eliminacao de Salto Redundante", insts[i], "(removida)");
                insts.erase(insts.begin() + static_cast<long>(i));
                --i;
                changed = true;
                continue;
            }
        }
    }

    return changed;
}

// ---------------------------------------------------------------------
// 6. Eliminação de Código Morto (Dead Code Elimination)
// ---------------------------------------------------------------------
bool TACOptimizer::passDeadCodeElimination(vector<string>& insts, int pass) {
    bool changed = false;

    // 6a. Remove instruções inalcançáveis imediatamente após um 'goto' até o próximo rótulo
    for (size_t i = 0; i < insts.size(); ++i) {
        ParsedInst p = parseInstruction(insts[i]);
        if (p.kind == InstKind::GOTO) {
            while (i + 1 < insts.size()) {
                ParsedInst nextP = parseInstruction(insts[i + 1]);
                if (nextP.kind == InstKind::LABEL) break;
                logOptimization(pass, "Eliminacao de Codigo Morto (Inalcancavel)", insts[i + 1], "(removida)");
                insts.erase(insts.begin() + static_cast<long>(i + 1));
                changed = true;
            }
        }
    }

    // 6b. Coleta todos os identificadores que são lidos em alguma instrução do programa
    unordered_set<string> readVars;
    unordered_set<string> usedLabels;
    for (const auto& line : insts) {
        ParsedInst p = parseInstruction(line);
        if (!p.src1.empty() && isIdentifier(p.src1)) readVars.insert(p.src1);
        if (!p.src2.empty() && isIdentifier(p.src2)) readVars.insert(p.src2);
        if ((p.kind == InstKind::GOTO || p.kind == InstKind::IF_FALSE) && !p.label.empty()) {
            usedLabels.insert(p.label);
        }
    }

    // Remove atribuições a variáveis/temporários que nunca são lidos em lugar algum
    for (size_t i = 0; i < insts.size(); ++i) {
        ParsedInst p = parseInstruction(insts[i]);
        if ((p.kind == InstKind::ASSIGN_COPY || p.kind == InstKind::ASSIGN_BINOP) &&
            readVars.find(p.dest) == readVars.end()) {
            logOptimization(pass, "Eliminacao de Codigo Morto (Variavel Nao Utilizada)", insts[i], "(removida)");
            insts.erase(insts.begin() + static_cast<long>(i));
            --i;
            changed = true;
        }
    }

    // 6c. Remove atribuições sobrescritas no mesmo bloco básico antes de qualquer leitura
    for (size_t i = 0; i < insts.size(); ++i) {
        ParsedInst p = parseInstruction(insts[i]);
        if (p.kind != InstKind::ASSIGN_COPY && p.kind != InstKind::ASSIGN_BINOP) continue;

        bool overwrittenBeforeRead = false;
        for (size_t j = i + 1; j < insts.size(); ++j) {
            ParsedInst nextP = parseInstruction(insts[j]);
            if (nextP.kind == InstKind::LABEL || nextP.kind == InstKind::GOTO || nextP.kind == InstKind::IF_FALSE) {
                break;
            }
            if (nextP.src1 == p.dest || nextP.src2 == p.dest) {
                break; // Foi lida antes de ser sobrescrita
            }
            if ((nextP.kind == InstKind::ASSIGN_COPY || nextP.kind == InstKind::ASSIGN_BINOP) &&
                nextP.dest == p.dest) {
                overwrittenBeforeRead = true;
                break;
            }
        }

        if (overwrittenBeforeRead) {
            logOptimization(pass, "Eliminacao de Codigo Morto (Atribuicao Sobrescrita)", insts[i], "(removida)");
            insts.erase(insts.begin() + static_cast<long>(i));
            --i;
            changed = true;
        }
    }

    // 6d. Remove rótulos que não são referenciados por nenhum goto/ifFalse
    for (size_t i = 0; i < insts.size(); ++i) {
        ParsedInst p = parseInstruction(insts[i]);
        if (p.kind == InstKind::LABEL && usedLabels.find(p.label) == usedLabels.end()) {
            logOptimization(pass, "Eliminacao de Rotulo Nao Referenciado", insts[i], "(removido)");
            insts.erase(insts.begin() + static_cast<long>(i));
            --i;
            changed = true;
        }
    }

    return changed;
}

// ---------------------------------------------------------------------
// Pipeline Principal de Otimização (Ponto Fixo)
// ---------------------------------------------------------------------
vector<string> TACOptimizer::optimize(const vector<string>& instructions) {
    originalInstructions = instructions;
    optimizedInstructions = instructions;
    history.clear();

    const int maxPasses = 10;
    int pass = 1;
    while (pass <= maxPasses) {
        bool changed = false;
        changed |= passConstantFolding(optimizedInstructions, pass);
        changed |= passAlgebraicSimplification(optimizedInstructions, pass);
        changed |= passConstantAndCopyPropagation(optimizedInstructions, pass);
        changed |= passCommonSubexpressionElimination(optimizedInstructions, pass);
        changed |= passRedundantInstructions(optimizedInstructions, pass);
        changed |= passDeadCodeElimination(optimizedInstructions, pass);

        if (!changed) break;
        pass++;
    }

    return optimizedInstructions;
}

void TACOptimizer::printOptimizationLog() const {
    cout << "Transformacoes de Otimizacao Aplicadas (" << history.size() << "):\n";
    cout << "-----------------------------------------------------------------------------------------\n";
    if (history.empty()) {
        cout << "  Nenhuma transformacao adicional necessaria (codigo ja otimizado).\n";
    } else {
        for (size_t i = 0; i < history.size(); ++i) {
            const auto& rec = history[i];
            cout << "  [" << setw(2) << (i + 1) << "] (Passo " << rec.pass << ") "
                 << rec.technique << ":\n"
                 << "       Antes : " << rec.before << "\n"
                 << "       Depois: " << rec.after << "\n";
        }
    }
    cout << "-----------------------------------------------------------------------------------------\n\n";
}

void TACOptimizer::printOptimizedInstructions() const {
    cout << "Saida TAC Otimizado:\n";
    for (const auto& inst : optimizedInstructions) {
        cout << inst << "\n";
    }
    cout << "\n";
}

void TACOptimizer::printComparison() const {
    cout << "Comparativo de Codigo Intermediario (Antes vs Depois da Otimizacao):\n";
    cout << "-------------------------------------------------------------------------\n";
    cout << "| " << left << setw(32) << "ANTES DA OTIMIZACAO"
         << " | " << setw(34) << "DEPOIS DA OTIMIZACAO" << " |\n";
    cout << "-------------------------------------------------------------------------\n";

    size_t maxLines = max(originalInstructions.size(), optimizedInstructions.size());
    for (size_t i = 0; i < maxLines; ++i) {
        string before = (i < originalInstructions.size()) ? originalInstructions[i] : "";
        string after  = (i < optimizedInstructions.size()) ? optimizedInstructions[i] : "";
        cout << "| " << left << setw(32) << before
             << " | " << setw(34) << after << " |\n";
    }

    cout << "-------------------------------------------------------------------------\n";
    cout << "Resumo: " << originalInstructions.size() << " instrucoes originais -> "
         << optimizedInstructions.size() << " instrucoes apos otimizacao ";
    if (!originalInstructions.empty()) {
        int removed = static_cast<int>(originalInstructions.size()) - static_cast<int>(optimizedInstructions.size());
        cout << "(" << removed << " instrucoes eliminadas).\n";
    } else {
        cout << "\n";
    }
}
