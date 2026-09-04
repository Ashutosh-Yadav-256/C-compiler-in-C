#ifndef IR_H
#define IR_H

#include "ast.h"
#include "arena.h"

typedef enum {
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
    IR_EQ, IR_NEQ, IR_LT, IR_GT, IR_LTE, IR_GTE,
    IR_BITAND, IR_BITOR, IR_BITXOR,
    IR_SHL, IR_SHR,
    IR_NEG, IR_NOT, IR_BITNOT,
    IR_MOV,
    IR_IMM,
    IR_LEA_STR,
    IR_LEA_VAR,
    IR_LOAD,
    IR_STORE,
    IR_JMP,
    IR_JMPZ,
    IR_LABEL,
    IR_PARAM,
    IR_ARG,
    IR_CALL,
    IR_RET
} ir_op_t;

typedef struct {
    ir_op_t op;
    int dest;
    int src1;
    int src2;
    long long imm;
    const char *name;
    size_t name_len;
} ir_instr_t;

typedef struct {
    const char *name;
    size_t name_len;
    ir_instr_t *instrs;
    size_t instr_count;
    size_t instr_capacity;
    int vreg_count;
    size_t param_count;
    int epilogue_label;
} ir_func_t;

typedef struct {
    int id;
    const char *val;
    size_t len;
    const char *decoded;
    size_t decoded_len;
} ir_string_t;

typedef struct {
    ir_func_t **funcs;
    size_t func_count;

    ir_string_t *strings;
    size_t string_count;
    size_t string_capacity;
} ir_program_t;

ir_program_t *ir_build(ast_node_t *prog_ast, arena_t *arena);

#endif

