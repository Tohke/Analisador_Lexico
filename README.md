# Compilador MicroRust / MicroC (`.mr`)

Compilador completo, pedagógico e modular desenvolvido em **C++17**, implementando todas as fases clássicas de compilação: análise léxica, análise sintática com recuperação de erros (*Panic Mode*), construção da Árvore Sintática Abstrata (AST), análise semântica com verificação de tipos e controle de escopo, tradução dirigida por sintaxe (RPN e TAC), otimização iterativa de código intermediário, reconstrução de código (*Pretty Printer*) e **máquina virtual de execução (TAC VM)** para atender integralmente ao requisito de execução do programa.

---

## 1. Gramática da Linguagem MicroRust (`.mr`)

A linguagem **MicroRust** baseia-se na sintaxe de Rust com tipagem estática e declaração de variáveis via `let` / `let mut`.

### 1.1 Notação EBNF

```ebnf
Program         ::= Statement*

Statement       ::= LetDecl
                  | Assignment
                  | IfStmt
                  | WhileStmt
                  | ForStmt
                  | PrintStmt

LetDecl         ::= "let" [ "mut" ] ID [ ":" Type ] [ "=" Expr ] ";"
Assignment      ::= ID "=" Expr ";"
IfStmt          ::= "if" Expr "{" Statement* "}" [ "else" "{" Statement* "}" ]
WhileStmt       ::= "while" Expr "{" Statement* "}"
ForStmt         ::= "for" ID "in" Expr ".." Expr "{" Statement* "}"
PrintStmt       ::= "println" "!" "(" STRING [ "," Expr ] ")" ";"

Expr            ::= LogicalOr
LogicalOr       ::= LogicalAnd ( "||" LogicalAnd )*
LogicalAnd      ::= Equality ( "&&" Equality )*
Equality        ::= Relational ( ( "==" | "!=" ) Relational )*
Relational      ::= Additive ( ( "<" | "<=" | ">" | ">=" ) Additive )*
Additive        ::= Term ( ( "+" | "-" ) Term )*
Term            ::= Factor ( ( "*" | "/" ) Factor )*
Factor          ::= NUM
                  | ID
                  | "(" Expr ")"

Type            ::= "i32" | "u32" | "f32" | "bool" | "char" | "str"
```

### 1.2 Léxico e Tokens

| Categoria | Padrão / Exemplos | Token |
| :--- | :--- | :--- |
| **Palavras-chave** | `let`, `mut`, `if`, `else`, `while`, `for`, `in`, `fn`, `match`, `println` | `T_LET`, `T_MUT`, `T_IF`, etc. |
| **Tipos Primitivos** | `i32`, `u32`, `f32`, `bool`, `char`, `str` | `T_TYPE` |
| **Identificadores** | `[a-zA-Z_][a-zA-Z0-9_]*` | `T_ID` |
| **Literais Numéricos** | `[0-9]+` ou `[0-9]+\.[0-9]+` | `T_NUM` |
| **Literais de Texto** | `"[^"]*"` | `T_STRING` |
| **Operadores Aritméticos** | `+`, `-`, `*`, `/` | `T_PLUS`, `T_MINUS`, `T_MULT`, `T_DIV` |
| **Operadores Relacionais** | `==`, `!=`, `<`, `<=`, `>`, `>=` | `T_EQ`, `T_NE`, `T_LT`, `T_LE`, etc. |
| **Operadores Lógicos** | `&&`, `\|\|` | `T_AND`, `T_OR` |
| **Delimitadores** | `(`, `)`, `{`, `}`, `;`, `:`, `,`, `->`, `=>` | `T_LPAREN`, `T_RBRACE`, etc. |
| **Comentários** | `// linha` e `/* bloco */` | *(descartados pelo scanner)* |

---

## 2. Arquitetura e Etapas do Compilador

O fluxo do compilador abrange da entrada de código até a execução final:

```
 Código-fonte (.mr)
          │
          ▼
   [ 1. Lexer ]           ──> Stream de Tokens (com rastreamento de linhas)
          │
          ▼
   [ 2. Parser ]          ──> Árvore Sintática Abstrata (AST) [c/ Panic Mode]
          │
          ▼
 [ 3. SemanticAnalyzer ]  ──> Tabela de Símbolos, Escopo, Tipos e Mutabilidade
          │
          ▼
   [ 4. SDT: RPN ]        ──> Notação Polonesa Reversa (Postfix)
          │
          ▼
   [ 5. SDT: TAC ]        ──> Código Intermediário (Three-Address Code)
          │
          ▼
 [ 6. TACOptimizer ]      ──> TAC Otimizado (Folding, Propagação, CSE, DCE)
          │
          ▼
 [ 7. PrettyPrinter ]     ──> Reconstrução Canônica de Código
          │
          ▼
 [ 8. TACInterpreter ]    ──> BACK-END / EXECUÇÃO (TAC VM e Estado da Memória)
```

### Detalhamento das Etapas

1. **Análise Léxica (`Lexer.cpp` / `Scanner`)**:
   - Converte o texto fonte em stream de tokens;
   - Trata comentários de linha (`//`) e bloco (`/* */`);
   - Rastreia números de linha e identifica caracteres desconhecidos.
2. **Análise Sintática (`Parser.cpp` / `AST.cpp`)**:
   - Descendente recursivo preditivo;
   - Constrói a AST com nós parametrizados;
   - Implementa recuperação de erros por **Panic Mode** com sincronização em `;` e palavras-chave de comandos.
3. **Análise Semântica (`SemanticAnalyzer.cpp`)**:
   - Visitor na AST mantendo pilha de escopos aninhados (`vector<unordered_map>`);
   - Valida declaração prévia, redeclaração no mesmo escopo e compatibilidade de tipos;
   - Garante regra estrita de imutabilidade (erro ao reatribuir sem `mut`).
4. **Tradução Dirigida por Sintaxe - SDT (`SDT.cpp`)**:
   - **PostfixTranslator**: Expressões em Notação Polonesa Reversa (RPN);
   - **TACTranslator**: Emite Código de Três Endereços (TAC) com temporários reutilizáveis (`t1, t2`) e rótulos (`L1, L2`).
5. **Otimização de Código Intermediário (`Optimizer.cpp`)**:
   - Executa múltiplos passes em ponto fixo aplicando:
     - *Constant Folding* (avaliação de operações constantes);
     - *Constant & Copy Propagation* (substituição de variáveis por seus valores conhecidos);
     - *Algebraic Simplification* (`x * 1 -> x`, `x + 0 -> x`);
     - *Common Subexpression Elimination - CSE* (reaproveitamento de expressões idênticas);
     - *Dead Code Elimination - DCE* (remoção de variáveis e atribuições sem uso posterior);
     - *Redundant Temporary Elimination* (colapso de temporários intermediários).
6. **Pretty Printer (`SDT.cpp`)**:
   - Formata e reconstrói o código-fonte canônico a partir da AST.
7. **Back-end de Execução (`Interpreter.cpp` / `TAC VM`)**:
   - Máquina virtual que interpreta o TAC otimizado;
   - Gerencia memória de variáveis e temporários, saltos de rótulos (`goto`, `ifFalse`) e saídas do `println!`;
   - Apresenta as saídas reais do programa e o estado final da memória.

---

## 3. Instruções para Compilação e Execução

### 3.1 Pré-requisitos
- Compilador C++17 (`g++` versão 8+ ou `clang++`).
- Utilitário `make`.
- Linux, macOS ou Windows (MinGW).

### 3.2 Compilando o Projeto
```bash
make clean
make
```

### 3.3 Executando um Arquivo `.mr`
O compilador aceita qualquer arquivo `.mr` diretamente via linha de comando:

```bash
# Executar um dos exemplos fornecidos:
./bin/compiler exemplos/exemplo1_fluxo.mr

# Executar o teste de otimizações:
./bin/compiler exemplos/exemplo2_otimizacao.mr

# Executar com erro sintático (demonstração do Panic Mode):
./bin/compiler exemplos/exemplo3_erro_sintatico.mr

# Executar com erro semântico (demonstração da checagem de tipos e mutabilidade):
./bin/compiler exemplos/exemplo4_erro_semantico.mr
```

Se nenhum argumento for passado (`make run` ou `./bin/compiler`), o compilador executa um programa padrão embutido.

---

## 4. Programas de Teste em MicroRust (`exemplos/`)

O repositório já inclui suíte de testes na pasta `exemplos/`:

### `exemplos/exemplo1_fluxo.mr` (Fluxo de Controle e Execução)
```rust
let limite: i32 = 5;
let mut contador: i32 = 0;
let passo = 1;

while contador < limite {
    contador = contador + passo;
}

if contador == limite {
    let sucesso = 100;
    println!("{}", contador);
    println!("{}", sucesso);
} else {
    let falha = 0;
    println!("{}", falha);
}
```
**Resultado da Execução:** Imprime `5` e `100`.

### `exemplos/exemplo2_otimizacao.mr` (Otimizações Extensivas)
```rust
let base: i32 = 10 + 5;
let fator: i32 = base * 1;
let mut x: i32 = 20;
let a = x + fator;
let b = x + fator;
let codigo_inutil = 999;
let resultado = a + b;
println!("{}", resultado);
```
**Resultado da Otimização:** As 13 instruções TAC originais são reduzidas a uma única instrução `print 70` através de Folding + Propagação + CSE + DCE, imprimindo `70` na execução.

### `exemplos/exemplo3_erro_sintatico.mr` (Recuperação Panic Mode)
Demonstra múltiplos erros de sintaxe (expressão incompleta e bloco `if` sem chave `{`) e a recuperação sincronizada do parser.

### `exemplos/exemplo4_erro_semantico.mr` (Checagem Semântica)
Demonstra a detecção de:
- Modificação de variável imutável (sem `mut`);
- Acesso a identificador não declarado;
- Redeclaração no mesmo escopo.

---

## 5. Relatório Técnico com Principais Decisões

1. **Entrada de Arquivos Flexível**: O compilador recebe arquivos `.mr` via argumento CLI com validação e fallback para código padrão caso executado sem argumentos.
2. **Gerenciamento de Memória com Smart Pointers**: AST construída usando `std::shared_ptr<ASTNode>`, garantindo desalocação automática e segurança no compartilhamento entre múltiplos Visitors.
3. **Padrão Visitor desacoplado**: As fases de SDT, Pretty Printer e Análise Semântica operam de forma isolada sobre a mesma estrutura de AST.
4. **Pilha de Escopos Dinâmica**: A `SymbolTable` permite aninhamento arbritário de blocos (`vector<unordered_map>`), garantindo isolamento local e histórico completo para auditoria.
5. **Otimizador em Ponto Fixo**: O `TACOptimizer` itera sobre as instruções TAC até que nenhum passo adicional produza mudanças, gerando log detalhado de cada transformação e comparativo Antes/Depois.
6. **Back-end com TAC VM Integrado**: Uma Máquina Virtual dedicada resolve o endereçamento de desvios (`goto`, `ifFalse`), temporários e variáveis, viabilizando a execução completa do programa compilado.

---

## 6. Apresentação e Demonstração Prática

Para demonstrar o projeto completo aos professores:

1. Compile o projeto:
   ```bash
   make clean && make
   ```
2. Demonstre o fluxo completo até a execução com `exemplo1_fluxo.mr`:
   ```bash
   ./bin/compiler exemplos/exemplo1_fluxo.mr
   ```
   *Mostre:* Tokens $\to$ AST $\to$ Tabela de Símbolos $\to$ RPN $\to$ TAC Original $\to$ Log de Otimizações $\to$ TAC Otimizado $\to$ Execução pela TAC VM imprimindo `5` e `100`.

3. Demonstre a eficiência do otimizador com `exemplo2_otimizacao.mr`:
   ```bash
   ./bin/compiler exemplos/exemplo2_otimizacao.mr
   ```
   *Mostre:* Tabela comparativa reduzindo 13 instruções para 1 instrução (`print 70`), com saída executada = `70`.

4. Demonstre o tratamento de erros:
   ```bash
   ./bin/compiler exemplos/exemplo3_erro_sintatico.mr
   ./bin/compiler exemplos/exemplo4_erro_semantico.mr
   ```
