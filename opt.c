#include "opt.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void opt_run_passes(ir_program_t *prog) {
    if (!prog) return;

    for (size_t f_idx = 0; f_idx < prog->func_count; f_idx++) {
        ir_func_t *func = prog->funcs[f_idx];
        if (!func || func->vreg_count == 0) continue;

        bool *is_const = calloc(func->vreg_count, sizeof(bool));
        long long *const_vals = calloc(func->vreg_count, sizeof(long long));
        if (!is_const || !const_vals) {
            free(is_const);
            free(const_vals);
            continue;
        }

        bool changed;
        do {
            changed = false;
            for (size_t i = 0; i < (size_t)func->vreg_count; i++) {
                is_const[i] = false;
            }

            for (size_t i = 0; i < func->instr_count; i++) {
                ir_instr_t *ins = &func->instrs[i];

                if (ins->op == IR_LABEL || ins->op == IR_JMP || ins->op == IR_JMPZ ||
                    ins->op == IR_CALL || ins->op == IR_STORE || ins->op == IR_RET) {
                    for (size_t j = 0; j < (size_t)func->vreg_count; j++) {
                        is_const[j] = false;
                    }
                    continue;
                }

                if (ins->op == IR_IMM) {
                    if (ins->dest >= 0 && ins->dest < func->vreg_count) {
                        is_const[ins->dest] = true;
                        const_vals[ins->dest] = ins->imm;
                    }
                } else if (ins->op == IR_MOV) {
                    if (ins->src1 >= 0 && ins->src1 < func->vreg_count && is_const[ins->src1]) {
                        if (ins->dest >= 0 && ins->dest < func->vreg_count) {
                            is_const[ins->dest] = true;
                            const_vals[ins->dest] = const_vals[ins->src1];

                            ins->op = IR_IMM;
                            ins->imm = const_vals[ins->dest];
                            ins->src1 = -1;
                            changed = true;
                        }
                    } else if (ins->dest >= 0 && ins->dest < func->vreg_count) {
                        is_const[ins->dest] = false;
                    }
                } else if (ins->op >= IR_ADD && ins->op <= IR_SHR) {
                    bool c1 = (ins->src1 >= 0 && ins->src1 < func->vreg_count && is_const[ins->src1]);
                    bool c2 = (ins->src2 >= 0 && ins->src2 < func->vreg_count && is_const[ins->src2]);

                    if (c1 && c2 && ins->dest >= 0 && ins->dest < func->vreg_count) {
                        long long v1 = const_vals[ins->src1];
                        long long v2 = const_vals[ins->src2];
                        long long res = 0;
                        bool valid = true;

                        switch (ins->op) {
                            case IR_ADD: res = v1 + v2; break;
                            case IR_SUB: res = v1 - v2; break;
                            case IR_MUL: res = v1 * v2; break;
                            case IR_DIV: if (v2 != 0) res = v1 / v2; else valid = false; break;
                            case IR_MOD: if (v2 != 0) res = v1 % v2; else valid = false; break;
                            case IR_EQ: res = (v1 == v2); break;
                            case IR_NEQ: res = (v1 != v2); break;
                            case IR_LT: res = (v1 < v2); break;
                            case IR_GT: res = (v1 > v2); break;
                            case IR_LTE: res = (v1 <= v2); break;
                            case IR_GTE: res = (v1 >= v2); break;
                            case IR_BITAND: res = (v1 & v2); break;
                            case IR_BITOR: res = (v1 | v2); break;
                            case IR_BITXOR: res = (v1 ^ v2); break;
                            case IR_SHL: res = (v1 << (v2 & 63)); break;
                            case IR_SHR: res = (v1 >> (v2 & 63)); break;
                            default: valid = false; break;
                        }

                        if (valid) {
                            is_const[ins->dest] = true;
                            const_vals[ins->dest] = res;

                            ins->op = IR_IMM;
                            ins->imm = res;
                            ins->src1 = -1;
                            ins->src2 = -1;
                            changed = true;
                        }
                    } else if (ins->dest >= 0 && ins->dest < func->vreg_count) {
                        is_const[ins->dest] = false;
                    }
                } else if (ins->op >= IR_NEG && ins->op <= IR_BITNOT) {
                    if (ins->src1 >= 0 && ins->src1 < func->vreg_count && is_const[ins->src1]) {
                        long long v1 = const_vals[ins->src1];
                        long long res = 0;
                        switch (ins->op) {
                            case IR_NEG: res = -v1; break;
                            case IR_NOT: res = !v1; break;
                            case IR_BITNOT: res = ~v1; break;
                            default: break;
                        }
                        if (ins->dest >= 0 && ins->dest < func->vreg_count) {
                            is_const[ins->dest] = true;
                            const_vals[ins->dest] = res;

                            ins->op = IR_IMM;
                            ins->imm = res;
                            ins->src1 = -1;
                            changed = true;
                        }
                    } else if (ins->dest >= 0 && ins->dest < func->vreg_count) {
                        is_const[ins->dest] = false;
                    }
                } else if (ins->dest >= 0 && ins->dest < func->vreg_count) {
                    is_const[ins->dest] = false;
                }
            }
        } while (changed);

        free(is_const);
        free(const_vals);
    }
}

