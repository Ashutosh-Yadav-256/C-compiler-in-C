# C-Compiler

A native x86-64 C compiler written in pure C from first principles. Built around a modular systems architecture featuring dynamic chained chunk arena allocation, a 15-precedence typed Abstract Syntax Tree (AST), three-address code (TAC) intermediate representation, control-flow-safe constant optimization, and System V AMD64 ABI compliant code generation.

[![Language](https://img.shields.io/badge/Language-Pure%20C%20(C11)-00599C?style=flat-square)](https://github.com/Ashutosh-Yadav-256/C-compiler-in-C)
[![Target Architecture](https://img.shields.io/badge/Target-x86--64%20(AMD64)-B45309?style=flat-square)](https://github.com/Ashutosh-Yadav-256/C-compiler-in-C)
[![Assembly Output](https://img.shields.io/badge/Assembly-NASM%20(Intel%20Syntax)-2D6A4F?style=flat-square)](https://github.com/Ashutosh-Yadav-256/C-compiler-in-C)
[![ABI Standard](https://img.shields.io/badge/ABI-System%20V%20AMD64-3B3030?style=flat-square)](https://github.com/Ashutosh-Yadav-256/C-compiler-in-C)
[![Web Playground](https://img.shields.io/badge/Live%20Demo-ccompiler.site-0284C7?style=flat-square)](https://www.ccompiler.site)
[![License](https://img.shields.io/badge/License-MIT-gray?style=flat-square)](LICENSE)

> ### **Why build a compiler?**
> Developers use compilers every day without seeing what happens between source code and machine code. **C-Compiler** opens the black box of compilation, letting you explore the process step by step from lexical analysis and AST generation to IR, optimization, x86-64 assembly and execution.

---

## Interactive Navigation

<table>
  <tr>
    <td align="center" width="25%">
      <a href="#live-web-playground"><b>Web Playground</b></a><br>
      <sub>Interactive browser compiler</sub>
    </td>
    <td align="center" width="25%">
      <a href="#architectural-pipeline"><b>Architecture</b></a><br>
      <sub>6-stage compilation pipeline</sub>
    </td>
    <td align="center" width="25%">
      <a href="#interactive-compilation-walkthrough"><b>Walkthrough</b></a><br>
      <sub>Step-by-step code transforms</sub>
    </td>
    <td align="center" width="25%">
      <a href="#quick-start--installation"><b>Quick Start</b></a><br>
      <sub>1-line install & build guide</sub>
    </td>
  </tr>
  <tr>
    <td align="center" width="25%">
      <a href="#compiler-pipeline-deep-dive"><b>Pipeline Deep Dive</b></a><br>
      <sub>Lexer to x86-64 codegen</sub>
    </td>
    <td align="center" width="25%">
      <a href="#language-support--abi-matrix"><b>Language Matrix</b></a><br>
      <sub>Types, control flow & syntax</sub>
    </td>
    <td align="center" width="25%">
      <a href="#test-suite--verification"><b>Test Suite</b></a><br>
      <sub>ASan, UBSan & integration tests</sub>
    </td>
    <td align="center" width="25%">
      <a href="#author--engineering"><b>Author</b></a><br>
      <sub>Engineering & LinkedIn profile</sub>
    </td>
  </tr>
</table>

---

## Live Web Playground

You can test, compile, inspect intermediate representations, and execute programs directly in the web browser without installing local dependencies:

- **Primary Custom Domain**: [https://www.ccompiler.site](https://www.ccompiler.site)
- **Cloudflare Edge Mirror**: [https://c-compiler-in-c.ashutoshyadav2102004.workers.dev](https://c-compiler-in-c.ashutoshyadav2102004.workers.dev)

The web explorer provides live tab switching across:
- **Program Output**: Sandbox VM virtual terminal execution.
- **x86-64 NASM**: Emitted Intel syntax assembly.
- **Three-Address IR**: Linearized TAC quad stream.
- **Typed AST**: Scoped hierarchical syntax tree.
- **Token Stream**: Lexical scanner output with line numbers.

---

## Architectural Pipeline

The compiler transforms pure C source code into runnable native machine code across six decoupled, strictly validated stages.

```text
+-------------------------------------------------------------------------------+
|                             C SOURCE CODE (.c)                                |
+-------------------------------------------------------------------------------+
                                       |
                                       v
+-------------------------------------------------------------------------------+
| 1. LEXICAL SCANNER (lexer.c)                                                  |
|    - Zero-copy buffer slicing                                                 |
|    - Multi-radix integer parsing (Dec, Hex, Bin, Oct)                         |
|    - Escape sequences (\n, \t, \r, \\, \0, \xHH)                              |
+-------------------------------------------------------------------------------+
                                       | Token Stream
                                       v
+-------------------------------------------------------------------------------+
| 2. RECURSIVE DESCENT PARSER (parser.c)                                        |
|    - 15-precedence Pratt / climbing expression parsing                        |
|    - Control flow (if-else, while, for, break, continue, return)              |
|    - Arena-allocated Typed Abstract Syntax Tree (AST)                         |
+-------------------------------------------------------------------------------+
                                       | Typed AST
                                       v
+-------------------------------------------------------------------------------+
| 3. SEMANTIC ANALYZER (sema.c)                                                 |
|    - Lexical block symbol resolution                                          |
|    - Type checking (int, char, long, void, pointer T*)                        |
|    - Lvalue validation and implicit pointer decay                             |
+-------------------------------------------------------------------------------+
                                       | Validated AST
                                       v
+-------------------------------------------------------------------------------+
| 4. THREE-ADDRESS IR GENERATOR (ir.c)                                          |
|    - Linear instruction stream (IR_ADD, IR_SUB, IR_LOAD, IR_STORE, etc.)      |
|    - Control-flow flattening with explicit label jumps                        |
|    - Dedicated function epilogue routing for early return handling            |
+-------------------------------------------------------------------------------+
                                       | Raw TAC IR
                                       v
+-------------------------------------------------------------------------------+
| 5. FLOW-SAFE OPTIMIZER (opt.c)                                                |
|    - Basic-block partitioned constant folding                                 |
|    - Local constant propagation without cross-block contamination             |
|    - Dead code and jump-to-jump simplification                                |
+-------------------------------------------------------------------------------+
                                       | Optimized TAC IR
                                       v
+-------------------------------------------------------------------------------+
| 6. x86-64 CODE GENERATOR (codegen.c)                                          |
|    - System V AMD64 ABI compliant register passing (RDI, RSI, RDX, RCX, R8, R9) |
|    - Strict 16-byte stack frame alignment for function calls                  |
|    - Variadic function AL register clear (AL=0 for printf)                    |
+-------------------------------------------------------------------------------+
                                       |
                                       v
+-------------------------------------------------------------------------------+
|                         NASM ASSEMBLY CODE (.asm)                             |
+-------------------------------------------------------------------------------+
                                       |
               [ NASM Assembler ] ---> [ Object File (.o) ]
                                                |
                      [ Linker (GCC / LD) ] ---> [ Native ELF Executable ]
```

---

## Interactive Compilation Walkthrough

Below is a complete end-to-end breakdown of how a small C program is transformed through every internal layer of the compiler.

### Input C Source Code

```c
int square(int x) {
    return x * x;
}

int main() {
    int val = 6;
    return square(val);
}
```

<details open>
<summary><b>Stage 1: Token Stream (Click to expand / collapse)</b></summary>

```text
[Line 1] TOKEN_INT        : "int"
[Line 1] TOKEN_IDENT      : "square"
[Line 1] TOKEN_LPAREN     : "("
[Line 1] TOKEN_INT        : "int"
[Line 1] TOKEN_IDENT      : "x"
[Line 1] TOKEN_RPAREN     : ")"
[Line 1] TOKEN_LBRACE     : "{"
[Line 2] TOKEN_RETURN     : "return"
[Line 2] TOKEN_IDENT      : "x"
[Line 2] TOKEN_STAR       : "*"
[Line 2] TOKEN_IDENT      : "x"
[Line 2] TOKEN_SEMICOLON  : ";"
[Line 3] TOKEN_RBRACE     : "}"
[Line 5] TOKEN_INT        : "int"
[Line 5] TOKEN_IDENT      : "main"
[Line 5] TOKEN_LPAREN     : "("
[Line 5] TOKEN_RPAREN     : ")"
[Line 5] TOKEN_LBRACE     : "{"
[Line 6] TOKEN_INT        : "int"
[Line 6] TOKEN_IDENT      : "val"
[Line 6] TOKEN_ASSIGN     : "="
[Line 6] TOKEN_INT_LIT    : "6" (val=6)
[Line 6] TOKEN_SEMICOLON  : ";"
[Line 7] TOKEN_RETURN     : "return"
[Line 7] TOKEN_IDENT      : "square"
[Line 7] TOKEN_LPAREN     : "("
[Line 7] TOKEN_IDENT      : "val"
[Line 7] TOKEN_RPAREN     : ")"
[Line 7] TOKEN_SEMICOLON  : ";"
[Line 8] TOKEN_RBRACE     : "}"
[Line 9] TOKEN_EOF        : ""
```

</details>

<details open>
<summary><b>Stage 2: Typed Abstract Syntax Tree (AST) (Click to expand / collapse)</b></summary>

```text
PROGRAM
|-- FUNCTION_DEF: square (return_type: int, params: [int x])
|   `-- BLOCK
|       `-- RETURN_STMT
|           `-- BINARY_OP: * (type: int)
|               |-- VAR: x (type: int)
|               `-- VAR: x (type: int)
`-- FUNCTION_DEF: main (return_type: int, params: [])
    `-- BLOCK
        |-- VAR_DECL: val (type: int) = INT_LITERAL: 6
        `-- RETURN_STMT
            `-- CALL: square (type: int)
                `-- ARGS:
                    `-- VAR: val (type: int)
```

</details>

<details open>
<summary><b>Stage 3: Three-Address Intermediate Representation (TAC IR) (Click to expand / collapse)</b></summary>

```text
FUNCTION square:
  t0 = LOAD_PARAM 0
  t1 = t0 * t0
  RET t1
  JMP .Lsquare_epilogue
.Lsquare_epilogue:
  EPILOGUE

FUNCTION main:
  var val = 6
  t2 = LOAD val
  t3 = CALL square, [t2]
  RET t3
  JMP .Lmain_epilogue
.Lmain_epilogue:
  EPILOGUE
```

</details>

<details open>
<summary><b>Stage 4: Emitted x86-64 NASM Assembly (Click to expand / collapse)</b></summary>

```nasm
global square
square:
    push rbp
    mov rbp, rsp
    mov [rbp-8], rdi
    mov eax, [rbp-8]
    imul eax, [rbp-8]
    jmp .Lsquare_epilogue
.Lsquare_epilogue:
    mov rsp, rbp
    pop rbp
    ret

global main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov dword [rbp-4], 6
    mov eax, [rbp-4]
    mov edi, eax
    call square
    jmp .Lmain_epilogue
.Lmain_epilogue:
    mov rsp, rbp
    pop rbp
    ret
```

</details>

<details open>
<summary><b>Stage 5: Execution Result (Click to expand / collapse)</b></summary>

```text
Execution Output:
========================================
Program returned exit code: 36
========================================
```

</details>

---

## Compiler Pipeline Deep Dive

<details>
<summary><b>1. Dynamic Chained Arena Allocator (arena.c / arena.h)</b></summary>
<br>

The compiler bypasses individual `malloc` and `free` operations across the AST and IR nodes by utilizing an arena memory allocator with fixed 64KB chained blocks.

```text
[ Arena Root ]
      |
      v
+--------------------------+    +--------------------------+    +--------------------------+
| Chunk 1 (64 KB)          |--->| Chunk 2 (64 KB)          |--->| Chunk 3 (64 KB)          |
| [used: 65536 / 65536]    |    | [used: 48210 / 65536]    |    | [used: 0 / 65536]        |
+--------------------------+    +--------------------------+    +--------------------------+
```

- **Allocation**: Pointer-bump allocation inside the active chunk (`O(1)` time complexity).
- **Expansion**: Seamlessly allocates a new 64KB chunk when capacity is reached.
- **Lifetime Management**: Complete AST and IR tear down happens in a single call to `arena_free()`.
- **Checkpoints**: Supports scoped rollback for speculative parsing paths.

</details>

<details>
<summary><b>2. Lexical Scanner (lexer.c / lexer.h)</b></summary>
<br>

The lexer scans the raw C character stream into discrete tokens without intermediate heap string copies.

- **Zero-Copy Slicing**: Identifiers and keywords reference the source buffer directly.
- **Radix Support**:
  - Decimal: `42`, `1000`
  - Hexadecimal: `0x2A`, `0xFF`
  - Octal: `052`, `0777`
  - Binary: `0b101010`
- **Escape Character Decoding**: `\n`, `\t`, `\r`, `\\`, `\'`, `\"`, `\0`, `\xHH`.
- **Token Operators**: Full set of C operators including pointer arrow `->`, pre/post increments `++`/`--`, bitwise shifts `<<`/`>>`, compound assignments `+=`, `-=`, `*=`, `/=`.

</details>

<details>
<summary><b>3. Recursive Descent Parser (parser.c / parser.h)</b></summary>
<br>

The parser constructs a strongly-typed Abstract Syntax Tree using recursive descent and precedence climbing across 15 standard operator levels.

- **Declarations**: Supports functions, primitive types (`int`, `char`, `long`, `void`), single/multi-level pointers (`int*`, `char**`), and arrays.
- **Statements**: Structured blocks, variable initializers, expression statements.
- **Control Flow**: `if` / `else` branches, `while` loops, `for` loops, `break`, `continue`, `return`.
- **Expressions**: Parenthesized grouping, function calls with multiple arguments, subscript access `arr[i]`, address-of `&x`, dereference `*p`, and `sizeof(type | expr)`.

</details>

<details>
<summary><b>4. Semantic Analysis & Symbol Table (sema.c / sema.h)</b></summary>
<br>

Semantic passes validate correctness, resolve identifiers to lexical scopes, and verify type rules before code lowering.

- **Lexical Scoping**: Shadowing rules across global, function, and nested block scopes.
- **Type Checking**: Type promotions, pointer arithmetic scaling factors, assignment type compatibility.
- **Lvalue Verification**: Ensures assignment targets and address-of `&` targets are valid modifiable lvalues.
- **Function Signatures**: Verifies parameter counts and argument types on call sites.

</details>

<details>
<summary><b>5. Three-Address Code IR Generator (ir.c / ir.h)</b></summary>
<br>

Lowers the hierarchical AST into a linearized stream of three-address instructions.

- **Instruction Set**: `IR_ADD`, `IR_SUB`, `IR_MUL`, `IR_DIV`, `IR_MOD`, `IR_AND`, `IR_OR`, `IR_XOR`, `IR_SHL`, `IR_SHR`, `IR_EQ`, `IR_NE`, `IR_LT`, `IR_LE`, `IR_GT`, `IR_GE`, `IR_LOAD`, `IR_STORE`, `IR_LEA_VAR`, `IR_PARAM`, `IR_CALL`, `IR_RET`, `IR_JMP`, `IR_JMP_IF_ZERO`, `IR_LABEL`, `IR_EPILOGUE`.
- **Control-Flow Flattening**: Converts high-level loops and branches into conditional jumps and forward/backward labels.
- **Dedicated Function Epilogues**: Routes all function return statements to `.L<func>_epilogue`, guaranteeing reliable frame restoration for functions with multiple early exits.

</details>

<details>
<summary><b>6. Flow-Safe Optimizer (opt.c / opt.h)</b></summary>
<br>

Applies intra-block optimizations while preserving strict control flow boundaries.

- **Basic Block Isolation**: Optimizations partition on labels and conditional jumps to prevent variable state corruption across branches.
- **Constant Folding**: Precomputes deterministic operations at compile time (e.g., `4 + 5 * 2` -> `14`).
- **Constant Propagation**: Tracks constant assignments within a single basic block.
- **Algebraic Simplification**: Eliminates identity operations (`x + 0`, `x * 1`, `x * 0`).

</details>

<details>
<summary><b>7. x86-64 NASM Code Generator (codegen.c / codegen.h)</b></summary>
<br>

Translates TAC IR directly into standard x86-64 Intel-syntax NASM assembly code following the System V AMD64 ABI.

- **Calling Convention**: Passes the first 6 integer/pointer arguments via registers:
  1. `RDI`
  2. `RSI`
  3. `RDX`
  4. `RCX`
  5. `R8`
  6. `R9`
- **Stack Spill & Alignment**: Automatically handles 7+ parameters via stack spill slots and enforces 16-byte stack frame alignment at all `call` sites.
- **Variadic ABI**: Clears `AL` (`xor eax, eax` / `mov al, 0`) before issuing calls to variadic standard functions such as `printf`.

</details>

---

## Language Support & ABI Matrix

### Language Construct Support

| Construct | Syntax Example | Status | Backend Implementation |
| :--- | :--- | :---: | :--- |
| **Data Types** | `int`, `char`, `long`, `void`, `int*`, `char**` | Complete | Sized stack slots and word registers (`RAX`, `EAX`, `AL`) |
| **Pointers** | `*p = 10;`, `int* ptr = &val;` | Complete | `IR_LOAD`, `IR_STORE`, and `IR_LEA_VAR` memory ops |
| **Arrays** | `int arr[10];`, `arr[i] = 42;` | Complete | Stack allocation with scaled pointer offset calculation |
| **Control Flow** | `if`, `else`, `while`, `for`, `break`, `continue` | Complete | Bounded jump labels with condition evaluation |
| **Early Returns** | `if (x < 0) return -1;` | Complete | Epilogue jump target routing with clean stack teardown |
| **Functions** | `int add(int a, int b) { ... }` | Complete | System V ABI register sequence with stack spill fallback |
| **Variadic Calls** | `printf("Val: %d\n", num);` | Complete | `al=0` vector register clear before external call |
| **Bitwise Ops** | `a & b`, `a | b`, `a ^ b`, `~a`, `a << 2`, `a >> 1` | Complete | Native x86 `and`, `or`, `xor`, `not`, `shl`, `sar` instructions |
| **Compound Assign** | `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=` | Complete | Read-modify-write IR instruction emission |
| **Sizeof Operator** | `sizeof(int)`, `sizeof(ptr)` | Complete | Compile-time constant resolution |

### Operator Precedence Hierarchy (15 Levels)

| Level | Operators | Associativity | Description |
| :---: | :--- | :---: | :--- |
| **1** | `()`, `[]`, `->`, `.` | Left to Right | Function calls, subscripting, member access |
| **2** | `!`, `~`, `++`, `--`, `+` (unary), `-` (unary), `*` (deref), `&` (addr), `sizeof` | Right to Left | Unary operators and type queries |
| **3** | `*`, `/`, `%` | Left to Right | Multiplicative arithmetic |
| **4** | `+`, `-` | Left to Right | Additive arithmetic |
| **5** | `<<`, `>>` | Left to Right | Bitwise shift left and right |
| **6** | `<`, `<=`, `>`, `>=` | Left to Right | Relational inequality comparisons |
| **7** | `==`, `!=` | Left to Right | Equality comparisons |
| **8** | `&` | Left to Right | Bitwise AND |
| **9** | `^` | Left to Right | Bitwise XOR |
| **10** | `|` | Left to Right | Bitwise OR |
| **11** | `&&` | Left to Right | Logical AND (short-circuiting) |
| **12** | `||` | Left to Right | Logical OR (short-circuiting) |
| **13** | `?:` | Right to Left | Ternary conditional |
| **14** | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `^=`, `|=`, `<<=`, `>>=` | Right to Left | Assignment and compound assignment |
| **15** | `,` | Left to Right | Comma sequencing |

---

## Quick Start & Installation

### Option 1: One-Line Automatic Install (Linux & macOS)

Run the automated installer to download, compile, and configure the compiler binary on your system:

```bash
curl -fsSL https://raw.githubusercontent.com/Ashutosh-Yadav-256/C-compiler-in-C/main/install.sh | bash
```

### Option 2: Build From Source

#### Prerequisites
- A C compiler (`gcc` or `clang`)
- `make`
- `nasm` (for assembling generated `.asm` files)
- `binutils` / `gcc` (for linking native executables)

```bash
git clone https://github.com/Ashutosh-Yadav-256/C-compiler-in-C.git
cd C-compiler-in-C
make release
```

### Option 3: Compile and Run Programs

Compile C source code to x86-64 NASM assembly:
```bash
./ccompiler tests/hello.c hello.asm
```

Assemble and link into a runnable native binary:
```bash
./buildasm.sh tests/hello.c hello_bin
```

Run the compiled executable:
```bash
./hello_bin
```

### CLI Command Options

```text
Usage: ./ccompiler [options] <input.c> [output.asm]

Options:
  -o <file>        Specify output assembly file name
  --dump-tokens    Print token stream to stdout
  --dump-ast       Print Abstract Syntax Tree to stdout
  --dump-ir        Print Three-Address Code IR to stdout
  -O0              Disable optimizer passes
  -O1              Enable constant folding and propagation (default)
  -v, --version    Display compiler version information
  -h, --help       Display this help message
```

---

## Test Suite & Verification

The project includes an automated test harness validating syntax parsing, code emission, and runtime behavior.

Run all integration tests:
```bash
make test
```

Run tests with AddressSanitizer and UndefinedBehaviorSanitizer:
```bash
make asan
./tests/run_tests.sh
```

### Test Coverage Verification Matrix

| Test Suite File | Feature Validated | Verification Criterion | Result |
| :--- | :--- | :--- | :---: |
| `tests/hello.c` | Process return code & entry | Exit code matches expected constant | `[ PASS ]` |
| `tests/arithmetic.c` | 15-level operator precedence | Correct evaluation order & signs | `[ PASS ]` |
| `tests/early_return.c` | Dedicated epilogue jumps | Stack frames clean on early exit | `[ PASS ]` |
| `tests/pointers.c` | `*p` indirection and `&x` address-of | Correct stack dereferencing | `[ PASS ]` |
| `tests/bitwise.c` | `&`, `|`, `^`, `~`, `<<`, `>>` | Accurate bitwise transformations | `[ PASS ]` |
| `tests/chars_strings.c` | Character literals and escape sequences | ASCII codes & pointer offsets | `[ PASS ]` |
| `tests/control_flow.c` | `while`, `for`, `break`, `continue` | Proper loop termination & jumps | `[ PASS ]` |
| `tests/functions.c` | 6+ argument calling convention & recursion | Fibonacci & register pass | `[ PASS ]` |
| `tests/const_branch_safety.c`| Optimizer basic block boundaries | No variable contamination | `[ PASS ]` |
| `tests/fizzbuzz.c` | Variadic `printf` integration | Loop output & `al=0` call | `[ PASS ]` |

---

## Repository Structure

```text
C-Compiler/
|-- docs/
|   `-- index.html            # Web compiler explorer & client-side runner
|-- tests/                    # Integration test suite & validation scripts
|   |-- arithmetic.c
|   |-- bitwise.c
|   |-- chars_strings.c
|   |-- const_branch_safety.c
|   |-- control_flow.c
|   |-- early_return.c
|   |-- fizzbuzz.c
|   |-- functions.c
|   |-- hello.c
|   |-- pointers.c
|   `-- run_tests.sh
|-- arena.c / arena.h         # Dynamic chained chunk arena memory allocator
|-- ast.h                     # Typed AST definitions and node constructors
|-- lexer.c / lexer.h         # Zero-copy lexical scanner
|-- parser.c / parser.h       # 15-precedence recursive descent parser
|-- sema.c / sema.h           # Semantic analyzer and lexical symbol table
|-- ir.c / ir.h               # Three-address code (TAC) intermediate representation
|-- opt.c / opt.h             # Basic-block partitioned flow-safe optimizer
|-- codegen.c / codegen.h     # System V AMD64 x86-64 NASM code generator
|-- main.c                    # Compiler CLI driver and command dispatcher
|-- Makefile                  # Build automation (debug, release, asan, test)
|-- buildasm.sh               # Assembler/linker driver script
|-- install.sh                # Automated installation script
|-- Dockerfile                # Containerized build & test environment
|-- wrangler.json             # Cloudflare edge deployment configuration
|-- LICENSE                   # MIT License
`-- README.md                 # Project documentation
```

---

## Author & Engineering

Built & Engineered by **[Ashutosh Yadav](https://www.linkedin.com/in/ashutoshyadav256/)**

- **LinkedIn**: [https://www.linkedin.com/in/ashutoshyadav256/](https://www.linkedin.com/in/ashutoshyadav256/)
- **GitHub Repository**: [https://github.com/Ashutosh-Yadav-256/C-compiler-in-C](https://github.com/Ashutosh-Yadav-256/C-compiler-in-C)
- **Live Compiler Web App**: [https://www.ccompiler.site](https://www.ccompiler.site)

---

## License

This project is open source and available under the [MIT License](LICENSE).
