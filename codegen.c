#include "codegen.h"

static int vreg_offset(int vreg) {
    return -8 * (vreg + 1);
}

static const char *param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

int cgen_emit_x86_64(FILE *out, ir_program_t *prog) {
    if (!prog) return 1;

    if (prog->string_count > 0) {
        fprintf(out, "section .rodata\n");
        for (size_t i = 0; i < prog->string_count; i++) {
            fprintf(out, ".STR%d: db ", prog->strings[i].id);
            const unsigned char *bytes = (const unsigned char *)(prog->strings[i].decoded ? prog->strings[i].decoded : prog->strings[i].val);
            size_t blen = prog->strings[i].decoded ? prog->strings[i].decoded_len : prog->strings[i].len;
            for (size_t b = 0; b < blen; b++) {
                fprintf(out, "%u, ", (unsigned int)bytes[b]);
            }
            fprintf(out, "0\n");
        }
    }

    for (size_t f_idx = 0; f_idx < prog->func_count; f_idx++) {
        ir_func_t *func = prog->funcs[f_idx];
        if (!func) continue;

        fprintf(out, "global %.*s\n", (int)func->name_len, func->name);
        fprintf(out, "section .text\n");
        fprintf(out, "%.*s:\n", (int)func->name_len, func->name);
        fprintf(out, "    push rbp\n");
        fprintf(out, "    mov rbp, rsp\n");

        int stack_size = func->vreg_count * 8;
        stack_size = (stack_size + 15) & ~15;
        if (stack_size > 0) {
            fprintf(out, "    sub rsp, %d\n", stack_size);
        }

        int pending_args[64];

        for (size_t i = 0; i < func->instr_count; i++) {
            ir_instr_t *ins = &func->instrs[i];
            switch (ins->op) {
                case IR_PARAM:
                    if (ins->imm < 6) {
                        fprintf(out, "    mov [rbp%+d], %s\n", vreg_offset(ins->dest), param_regs[ins->imm]);
                    } else {
                        int param_offset = 16 + (int)(ins->imm - 6) * 8;
                        fprintf(out, "    mov rax, [rbp+%d]\n", param_offset);
                        fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    }
                    break;
                case IR_ARG:
                    if (ins->imm < 64) {
                        pending_args[ins->imm] = ins->src1;
                    }
                    break;
                case IR_CALL: {
                    int arg_count = (int)ins->imm;
                    int stack_args = arg_count > 6 ? arg_count - 6 : 0;
                    int stack_padding = (stack_args % 2 != 0) ? 8 : 0;

                    if (stack_padding > 0) {
                        fprintf(out, "    sub rsp, %d\n", stack_padding);
                    }

                    for (int j = arg_count - 1; j >= 6; j--) {
                        fprintf(out, "    push qword [rbp%+d]\n", vreg_offset(pending_args[j]));
                    }

                    for (int j = 0; j < arg_count && j < 6; j++) {
                        fprintf(out, "    mov %s, [rbp%+d]\n", param_regs[j], vreg_offset(pending_args[j]));
                    }

                    fprintf(out, "    xor eax, eax\n");
                    fprintf(out, "    call %.*s\n", (int)ins->name_len, ins->name);

                    int bytes_to_remove = stack_args * 8 + stack_padding;
                    if (bytes_to_remove > 0) {
                        fprintf(out, "    add rsp, %d\n", bytes_to_remove);
                    }

                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                }
                case IR_IMM:
                    fprintf(out, "    mov qword [rbp%+d], %lld\n", vreg_offset(ins->dest), ins->imm);
                    break;
                case IR_LEA_STR:
                    fprintf(out, "    lea rax, [rel .STR%lld]\n", ins->imm);
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_LEA_VAR:
                    fprintf(out, "    lea rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_LOAD:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    mov rax, [rax]\n");
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_STORE:
                    fprintf(out, "    mov rcx, [rbp%+d]\n", vreg_offset(ins->dest));
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    mov [rcx], rax\n");
                    break;
                case IR_MOV:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_ADD:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    add rax, [rbp%+d]\n", vreg_offset(ins->src2));
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_SUB:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    sub rax, [rbp%+d]\n", vreg_offset(ins->src2));
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_MUL:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    imul rax, [rbp%+d]\n", vreg_offset(ins->src2));
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_DIV:
                case IR_MOD:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    cqo\n");
                    fprintf(out, "    idiv qword [rbp%+d]\n", vreg_offset(ins->src2));
                    if (ins->op == IR_DIV) {
                        fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    } else {
                        fprintf(out, "    mov [rbp%+d], rdx\n", vreg_offset(ins->dest));
                    }
                    break;
                case IR_RET:
                    if (ins->src1 >= 0) {
                        fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    } else {
                        fprintf(out, "    xor eax, eax\n");
                    }

                    fprintf(out, "    jmp .L%.*s_epilogue\n", (int)func->name_len, func->name);
                    break;
                case IR_EQ:
                case IR_NEQ:
                case IR_LT:
                case IR_GT:
                case IR_LTE:
                case IR_GTE:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    cmp rax, [rbp%+d]\n", vreg_offset(ins->src2));
                    if (ins->op == IR_EQ) fprintf(out, "    sete al\n");
                    else if (ins->op == IR_NEQ) fprintf(out, "    setne al\n");
                    else if (ins->op == IR_LT) fprintf(out, "    setl al\n");
                    else if (ins->op == IR_GT) fprintf(out, "    setg al\n");
                    else if (ins->op == IR_LTE) fprintf(out, "    setle al\n");
                    else if (ins->op == IR_GTE) fprintf(out, "    setge al\n");
                    fprintf(out, "    movzx rax, al\n");
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_BITAND:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    and rax, [rbp%+d]\n", vreg_offset(ins->src2));
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_BITOR:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    or rax, [rbp%+d]\n", vreg_offset(ins->src2));
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_BITXOR:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    xor rax, [rbp%+d]\n", vreg_offset(ins->src2));
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_SHL:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    mov rcx, [rbp%+d]\n", vreg_offset(ins->src2));
                    fprintf(out, "    shl rax, cl\n");
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_SHR:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    mov rcx, [rbp%+d]\n", vreg_offset(ins->src2));
                    fprintf(out, "    sar rax, cl\n");
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_NEG:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    neg rax\n");
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_NOT:
                    fprintf(out, "    cmp qword [rbp%+d], 0\n", vreg_offset(ins->src1));
                    fprintf(out, "    sete al\n");
                    fprintf(out, "    movzx rax, al\n");
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_BITNOT:
                    fprintf(out, "    mov rax, [rbp%+d]\n", vreg_offset(ins->src1));
                    fprintf(out, "    not rax\n");
                    fprintf(out, "    mov [rbp%+d], rax\n", vreg_offset(ins->dest));
                    break;
                case IR_LABEL:
                    fprintf(out, ".L%.*s_%lld:\n", (int)func->name_len, func->name, ins->imm);
                    break;
                case IR_JMP:
                    fprintf(out, "    jmp .L%.*s_%lld\n", (int)func->name_len, func->name, ins->imm);
                    break;
                case IR_JMPZ:
                    fprintf(out, "    cmp qword [rbp%+d], 0\n", vreg_offset(ins->src1));
                    fprintf(out, "    je .L%.*s_%lld\n", (int)func->name_len, func->name, ins->imm);
                    break;
            }
        }

        fprintf(out, ".L%.*s_epilogue:\n", (int)func->name_len, func->name);
        fprintf(out, "    mov rsp, rbp\n");
        fprintf(out, "    pop rbp\n");
        fprintf(out, "    ret\n");
    }

    return 0;
}

