# C-Compiler

A native x86-64 C compiler written in pure C. Built around a modular pipeline featuring dynamic chained arena allocation, a typed Abstract Syntax Tree (AST), three-address code (TAC) intermediate representation, control-flow-safe constant optimization, and System V AMD64 ABI compliance.

[Live Web Playground & Compiler Explorer](https://ashutosh-yadav-tech.github.io/C-Compiler/)

---

## Features

- **Lexer**: Zero-copy token scanning supporting decimal, hexadecimal (`0x`), binary (`0b`), and octal (`0`) integer literals, character literals (`'a'`), multi-character operators (`->`, `++`, `--`, `<<`, `>>`, `&&`, `||`, `+=`, `-=`), and escape sequences (`\n`, `\t`, `\r`, `\\`, `\0`, `\xHH`).
- **Parser**: 15-level operator precedence recursive descent parser supporting variable declarations, pointer indirection (`*p`), address-of (`&x`), array indexing (`arr[i]`), `sizeof`, and structured control flow (`if/else`, `while`, `for`, `break`, `continue`, `return`).
- **Type System & Semantic Validation**: Typed AST supporting `int`, `char`, `void`, `long`, and pointer types (`T*`). Enforces lvalue validation, scope tracking, and type compatibility.
- **Three-Address Code (TAC) IR**: Explicit intermediate representation with memory operations (`IR_LOAD`, `IR_STORE`, `IR_LEA_VAR`), dedicated function epilogues (`.L<func>_epilogue`), and short-circuit evaluation for boolean operators.
- **Optimizer**: Basic-block partitioned constant folding and propagation that isolates optimizations to prevent cross-branch variable corruption.
- **x86-64 NASM Code Generator**: System V AMD64 ABI compliant code generator passing up to 6 arguments in registers (`rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`), maintaining 16-byte stack frame alignment, setting `al=0` for variadic calls (`printf`), and generating clean function prologues/epilogues.
- **Memory Management**: Dynamic chained arena allocator with 64KB chunk growth, alignment guarantees, and checkpoint rollback (`arena_save` / `arena_restore`), eliminating per-node allocation overhead and memory leaks.
- **Interactive Web Playground**: In-browser compiler explorer running client-side with assembly, IR, AST, token inspection, and simulated execution with `stdout` capture.

---

## Architecture Pipeline

```text
Source Code (.c)
      |
      v
+------------------+
|      Lexer       |   Zero-copy token slicing, number/escape decoding
+------------------+
      |
      v
+------------------+
|      Parser      |   15-precedence recursive descent parsing
+------------------+
      |
      v
+------------------+
|  Semantic Pass   |   Scoped symbol tables, type validation, lvalue checks
+------------------+
      |
      v
+------------------+
|  Three-Addr IR   |   Flattened TAC instructions & dedicated epilogues
+------------------+
      |
      v
+------------------+
|    Optimizer     |   Basic block local value numbering & constant folding
+------------------+
      |
      v
+------------------+
|  x86-64 Codegen  |   System V AMD64 ABI NASM assembly emission
+------------------+
      |
      v
Assembly (.asm) -> NASM (.o) -> GCC -> Native Binary
```

---

## Installation & Quick Start

### 1. One-Line Install (Linux & macOS)

```bash
curl -fsSL https://raw.githubusercontent.com/Ashutosh-Yadav-256/C-compiler-in-C/main/install.sh | bash
```

### 2. Build From Source

Prerequisites: `gcc`, `make`, and optionally `nasm` for assembling.

```bash
git clone https://github.com/Ashutosh-Yadav-256/C-compiler-in-C.git
cd C-compiler-in-C
make release
```

### 3. Compile a C File to Assembly

```bash
./ccompiler input.c output.asm
```

### 4. Assemble and Link to an Executable

```bash
./buildasm.sh input.c output_bin
./output_bin
```

---

## Test Suite

The project includes an automated test harness covering all major language features:

```bash
make test

make asan
./run_tests_asan.sh
```

### Test Coverage

- `tests/hello.c`: Return values and process exit codes
- `tests/arithmetic.c`: Binary arithmetic and operator precedence
- `tests/early_return.c`: Early return from nested branches and recursion
- `tests/pointers.c`: Pointer assignment, stack dereferencing, and address-of
- `tests/bitwise.c`: Bitwise operations (`&`, `|`, `^`, `~`, `<<`, `>>`)
- `tests/chars_strings.c`: Character literals and string escapes
- `tests/const_branch_safety.c`: Optimizer branch isolation and constant safety
- `tests/functions.c`: Multi-argument functions and recursive Fibonacci
- `tests/fizzbuzz.c`: Loop control flow and variadic `printf` output

---

## Repository Structure

```text
C-Compiler/
|-- docs/
|   `-- index.html
|-- tests/
|-- arena.c / arena.h
|-- ast.h
|-- lexer.c / lexer.h
|-- parser.c / parser.h
|-- sema.c / sema.h
|-- ir.c / ir.h
|-- opt.c / opt.h
|-- codegen.c / codegen.h
|-- main.c
|-- Makefile
|-- buildasm.sh
|-- install.sh
|-- Dockerfile
`-- LICENSE
```

---

## Continuous Integration & Releases

- **GitHub Actions CI**: Every commit is built and tested under GCC with AddressSanitizer and UndefinedBehaviorSanitizer enabled.
- **GitHub Pages**: The documentation and interactive web playground are deployed automatically on every push to `main`.
- **Release Automation**: Tagging a release (`v*.*.*`) triggers automated multi-platform binary compilation and asset publishing for Linux (x86_64, aarch64), macOS, and Windows.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

