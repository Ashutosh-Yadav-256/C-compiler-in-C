# C Compiler Implementation Specification

## 1. Overview
This document defines the requirements, architecture, memory contracts, and coding standards for the native C compiler written in C. The compiler translates C source code into System V AMD64 ABI compliant x86-64 assembly text (`.asm`), which is assembled via NASM and linked via GCC. The pipeline is designed around a modular, decoupled architecture: Lexer -> Parser -> Typed AST -> Semantic Analyzer -> Three-Address Code (TAC) IR -> Basic Block Optimizer -> x86-64 NASM Code Generator. All components prioritize memory safety through dynamic chained arena allocation, algorithmic clarity, and deterministic compilation.

## 2. Goals & Success Criteria
- **Feature Completeness:** Compiles structured C programs with functions, recursion, pointers, type checking, bitwise operators, string literals, and variadic standard library calls (`printf`).
- **Correctness:** 100% pass rate on integration test suites; zero memory leaks reported by AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).
- **Compile-Time Performance:** High-speed single-pass tokenization and linear IR generation targeting 10,000+ lines of C per second per core.
- **Resource Predictability:** Zero individual node `malloc`/`free` overhead. Uses dynamic chained 64KB arena allocations with `arena_save` and `arena_restore` checkpointing.
- **Reproducibility:** Deterministic, bit-for-bit identical assembly output for identical source inputs across all supported platforms.

## 3. Architecture & Component Contracts

### 3.1 Lexer (`lexer.h`, `lexer.c`)
- **Contract:** Zero-copy token scanning that converts UTF-8 source text into a sequential token stream.
- **Literals Supported:** Decimal, hexadecimal (`0x`/`0X`), binary (`0b`/`0B`), octal (`0...`), character literals (`'a'`), and string literals (`"..."`).
- **Escape Sequence Decoder:** Decodes `\n`, `\t`, `\r`, `\\`, `\'`, `\"`, `\0`, and hex bytes `\xHH`.
- **Operators:** Multi-character tokens (`==`, `!=`, `<=`, `>=`, `<<`, `>>`, `&&`, `||`, `->`, `++`, `--`, `+=`, `-=`, `*=`, `/=`).
- **Error Model:** Emits `TOK_ERROR` with file name, 1-indexed line number, and column.

### 3.2 Parser & AST (`parser.h`, `parser.c`, `ast.h`)
- **Contract:** 15-precedence recursive descent parser with panic-mode error synchronization.
- **Type Representation:** First-class `type_t` structures supporting `TYPE_VOID`, `TYPE_CHAR`, `TYPE_INT`, `TYPE_LONG`, `TYPE_PTR` (`T*`), `TYPE_ARRAY` (`T[N]`), and `TYPE_FUNC`.
- **Expressions:** Primary, postfix (`a[i]`, `f()`, `->`), unary/prefix (`&`, `*`, `+`, `-`, `!`, `~`, `sizeof`), casts, multiplicative, additive, shift, relational, equality, bitwise AND/XOR/OR, logical AND/OR, and assignment operators.
- **Statements:** Variable declarations with initialization, expression statements, block statements, `if/else`, `while`, `for`, `break`, `continue`, and `return`.
- **Memory:** Allocates all AST nodes directly within the compiler arena.

### 3.3 Semantic Analyzer (`sema.h`, `sema.c`)
- **Contract:** Two-pass AST validator managing scoped symbol tables (`symbol_t`).
- **Validation Rules:**
  - Type checking and implicit integer promotion.
  - Lvalue verification for assignment targets (`x = ...`, `*p = ...`).
  - Pointer arithmetic scaling based on base type sizes.
  - Return type matching against function signature.
  - Variable redeclaration and undeclared identifier diagnostics.

### 3.4 Intermediate Representation (`ir.h`, `ir.c`)
- **Contract:** Lowers typed AST into a flattened linear Three-Address Code (TAC) instruction array.
- **Instruction Set:** `IR_IMM`, `IR_MOV`, `IR_ADD`, `IR_SUB`, `IR_IMUL`, `IR_IDIV`, `IR_MOD`, `IR_AND`, `IR_OR`, `IR_XOR`, `IR_SHL`, `IR_SHR`, `IR_CMP_EQ`, `IR_CMP_NE`, `IR_CMP_LT`, `IR_CMP_GT`, `IR_CMP_LE`, `IR_CMP_GE`, `IR_JMP`, `IR_JMP_IF_ZERO`, `IR_CALL`, `IR_RET`, `IR_LOAD`, `IR_STORE`, `IR_LEA_VAR`.
- **Dedicated Epilogues:** Lowers `AST_RETURN` to emit `IR_RET` and jump unconditionally to `func->epilogue_label`, ensuring proper stack cleanup on early return.

### 3.5 Optimizer (`opt.h`, `opt.c`)
- **Contract:** Basic block isolated constant folding and dead code elimination.
- **Flow Safety:** Optimization state is partitioned strictly within basic blocks and invalidated at labels, conditional jumps, unconditional branches, and memory stores.

### 3.6 Code Generator (`codegen.h`, `codegen.c`)
- **Contract:** Translates TAC IR into System V AMD64 ABI compliant x86-64 NASM assembly text.
- **Register Convention:** Passes first 6 arguments in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`, with stack spilling for additional parameters.
- **Stack Alignment:** Maintains 16-byte alignment before `call` instructions.
- **Variadic Functions:** Clears `al` (`xor eax, eax`) before calling variadic functions like `printf`.
- **Epilogue Generation:** Emits dedicated `.L<func>_epilogue: mov rsp, rbp; pop rbp; ret` per function.

### 3.7 Memory Arena (`arena.h`, `arena.c`)
- **Contract:** Geometric dynamic chained chunk allocator with `uintptr_t` alignment.
- **Lifecycle:** Replaces flat buffers with chained 64KB blocks. Supports `arena_save` and `arena_restore` for scope-level rollback and O(1) bulk destruction at program exit.

---

## 4. Implementation & Code Quality Rules

1. **Coding Style:** K&R bracing, 4-space indentation, no tab characters, 100-character column line wraps.
2. **Error Handling:** Explicit error returns and descriptive diagnostic messages with line and column tracking.
3. **Memory Ownership:** Zero direct `malloc`/`free` calls inside parser, sema, IR, or codegen passes. All compiler data structures belong to the arena.
4. **Safety Verification:** Every change must compile cleanly with `-Wall -Wextra -Werror` and pass AddressSanitizer and UndefinedBehaviorSanitizer without warnings.

---

## 5. Build, Test & CI Specifications

- **Build Targets:**
  - `make all` / `make release`: Optimized production binary (`-O2`).
  - `make debug`: Debug build with symbol table (`-g -O0`).
  - `make asan`: Sanitizer build (`-fsanitize=address,undefined`).
  - `make test`: Automated test suite execution.
- **Continuous Integration:** GitHub Actions builds across Ubuntu, macOS, and Windows with multi-platform artifact packaging.
