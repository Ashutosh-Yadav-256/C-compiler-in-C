#include "ir.h"
#include <string.h>

typedef struct {
    ir_func_t *func;
    ir_program_t *prog;
    arena_t *arena;
    int next_label;
    int loop_start;
    int loop_end;
} ir_builder_t;

static int new_label(ir_builder_t *b) {
    return ++b->next_label;
}

static int new_vreg(ir_builder_t *b) {
    return b->func->vreg_count++;
}

static void emit(ir_builder_t *b, ir_instr_t instr) {
    if (!b || !b->func) return;
    if (b->func->instr_count == b->func->instr_capacity) {
        size_t new_cap = b->func->instr_capacity == 0 ? 32 : b->func->instr_capacity * 2;
        ir_instr_t *new_instrs = arena_alloc(b->arena, new_cap * sizeof(ir_instr_t));
        if (!new_instrs) return;
        if (b->func->instrs) {
            memcpy(new_instrs, b->func->instrs, b->func->instr_count * sizeof(ir_instr_t));
        }
        b->func->instrs = new_instrs;
        b->func->instr_capacity = new_cap;
    }
    if (b->func->instrs) {
        b->func->instrs[b->func->instr_count++] = instr;
    }
}

static int build_expr(ir_builder_t *b, ast_node_t *expr) {
    if (!expr) return -1;

    if (expr->kind == AST_INT_LITERAL) {
        int res = new_vreg(b);
        ir_instr_t instr = { .op = IR_IMM, .dest = res, .imm = expr->int_literal.value };
        emit(b, instr);
        return res;
    }

    if (expr->kind == AST_CHAR_LITERAL) {
        int res = new_vreg(b);
        ir_instr_t instr = { .op = IR_IMM, .dest = res, .imm = (unsigned char)expr->char_literal.value };
        emit(b, instr);
        return res;
    }

    if (expr->kind == AST_STRING_LITERAL) {
        int str_id = -1;
        const char *dec = expr->string_literal.decoded ? expr->string_literal.decoded : expr->string_literal.val;
        size_t dec_len = expr->string_literal.decoded ? expr->string_literal.decoded_len : expr->string_literal.len;

        for (size_t i = 0; i < b->prog->string_count; i++) {
            if (b->prog->strings[i].decoded_len == dec_len &&
                memcmp(b->prog->strings[i].decoded, dec, dec_len) == 0) {
                str_id = b->prog->strings[i].id;
                break;
            }
        }
        if (str_id == -1) {
            str_id = (int)b->prog->string_count;
            if (b->prog->string_count == b->prog->string_capacity) {
                size_t new_cap = b->prog->string_capacity == 0 ? 8 : b->prog->string_capacity * 2;
                ir_string_t *new_strings = arena_alloc(b->arena, new_cap * sizeof(ir_string_t));
                if (!new_strings) return -1;
                if (b->prog->strings) {
                    memcpy(new_strings, b->prog->strings, b->prog->string_count * sizeof(ir_string_t));
                }
                b->prog->strings = new_strings;
                b->prog->string_capacity = new_cap;
            }
            if (b->prog->strings) {
                b->prog->strings[b->prog->string_count].id = str_id;
                b->prog->strings[b->prog->string_count].val = expr->string_literal.val;
                b->prog->strings[b->prog->string_count].len = expr->string_literal.len;
                b->prog->strings[b->prog->string_count].decoded = dec;
                b->prog->strings[b->prog->string_count].decoded_len = dec_len;
                b->prog->string_count++;
            }
        }

        int res = new_vreg(b);
        ir_instr_t instr = { .op = IR_LEA_STR, .dest = res, .imm = str_id };
        emit(b, instr);
        return res;
    }

    if (expr->kind == AST_VAR) {
        return expr->var.symbol_id;
    }

    if (expr->kind == AST_ADDR_OF) {
        if (expr->addr_of.operand && expr->addr_of.operand->kind == AST_VAR) {
            int res = new_vreg(b);
            ir_instr_t instr = { .op = IR_LEA_VAR, .dest = res, .src1 = expr->addr_of.operand->var.symbol_id };
            emit(b, instr);
            return res;
        }
        if (expr->addr_of.operand && expr->addr_of.operand->kind == AST_DEREF) {
            return build_expr(b, expr->addr_of.operand->deref.operand);
        }
    }

    if (expr->kind == AST_DEREF) {
        int ptr_vreg = build_expr(b, expr->deref.operand);
        int res = new_vreg(b);
        ir_instr_t instr = { .op = IR_LOAD, .dest = res, .src1 = ptr_vreg };
        emit(b, instr);
        return res;
    }

    if (expr->kind == AST_SIZEOF) {
        int res = new_vreg(b);
        size_t size = 8;
        if (expr->size_of.target_type) size = expr->size_of.target_type->size;
        else if (expr->size_of.expr && expr->size_of.expr->type) size = expr->size_of.expr->type->size;

        ir_instr_t instr = { .op = IR_IMM, .dest = res, .imm = (long long)size };
        emit(b, instr);
        return res;
    }

    if (expr->kind == AST_ASSIGN) {
        if (expr->assign.target && expr->assign.target->kind == AST_DEREF) {
            int ptr_vreg = build_expr(b, expr->assign.target->deref.operand);
            int val_vreg = build_expr(b, expr->assign.expr);
            ir_instr_t instr = { .op = IR_STORE, .dest = ptr_vreg, .src1 = val_vreg };
            emit(b, instr);
            return val_vreg;
        }

        int res = build_expr(b, expr->assign.expr);
        int dest_vreg = expr->assign.symbol_id;
        ir_instr_t instr = { .op = IR_MOV, .dest = dest_vreg, .src1 = res };
        emit(b, instr);
        return dest_vreg;
    }

    if (expr->kind == AST_BINOP) {
        if (expr->binop.op == BINOP_LOGICAL_AND) {
            int res = new_vreg(b);
            ir_instr_t init_instr = { .op = IR_IMM, .dest = res, .imm = 0 };
            emit(b, init_instr);

            int left = build_expr(b, expr->binop.left);
            int lbl_false = new_label(b);
            int lbl_end = new_label(b);

            ir_instr_t jmpz1 = { .op = IR_JMPZ, .src1 = left, .imm = lbl_false };
            emit(b, jmpz1);

            int right = build_expr(b, expr->binop.right);
            ir_instr_t jmpz2 = { .op = IR_JMPZ, .src1 = right, .imm = lbl_false };
            emit(b, jmpz2);

            ir_instr_t set_true = { .op = IR_IMM, .dest = res, .imm = 1 };
            emit(b, set_true);

            ir_instr_t jmp_end = { .op = IR_JMP, .imm = lbl_end };
            emit(b, jmp_end);

            ir_instr_t l_false = { .op = IR_LABEL, .imm = lbl_false };
            emit(b, l_false);
            ir_instr_t l_end = { .op = IR_LABEL, .imm = lbl_end };
            emit(b, l_end);
            return res;
        }

        if (expr->binop.op == BINOP_LOGICAL_OR) {
            int res = new_vreg(b);
            ir_instr_t init_instr = { .op = IR_IMM, .dest = res, .imm = 0 };
            emit(b, init_instr);

            int left = build_expr(b, expr->binop.left);
            int lbl_check_right = new_label(b);
            int lbl_true = new_label(b);
            int lbl_end = new_label(b);

            ir_instr_t jmpz = { .op = IR_JMPZ, .src1 = left, .imm = lbl_check_right };
            emit(b, jmpz);

            ir_instr_t jmp_true = { .op = IR_JMP, .imm = lbl_true };
            emit(b, jmp_true);

            ir_instr_t l_chk = { .op = IR_LABEL, .imm = lbl_check_right };
            emit(b, l_chk);

            int right = build_expr(b, expr->binop.right);
            ir_instr_t jmpz_right = { .op = IR_JMPZ, .src1 = right, .imm = lbl_end };
            emit(b, jmpz_right);

            ir_instr_t l_true = { .op = IR_LABEL, .imm = lbl_true };
            emit(b, l_true);
            ir_instr_t set_true = { .op = IR_IMM, .dest = res, .imm = 1 };
            emit(b, set_true);

            ir_instr_t l_end = { .op = IR_LABEL, .imm = lbl_end };
            emit(b, l_end);
            return res;
        }

        int left = build_expr(b, expr->binop.left);
        int right = build_expr(b, expr->binop.right);

        if ((expr->binop.op == BINOP_ADD || expr->binop.op == BINOP_SUB) &&
            expr->binop.left->type && expr->binop.left->type->kind == TYPE_PTR &&
            expr->binop.left->type->ptr.base && expr->binop.left->type->ptr.base->size > 1) {
            size_t base_size = expr->binop.left->type->ptr.base->size;
            int scale_vreg = new_vreg(b);
            ir_instr_t imm_ins = { .op = IR_IMM, .dest = scale_vreg, .imm = (long long)base_size };
            emit(b, imm_ins);

            int scaled_right = new_vreg(b);
            ir_instr_t mul_ins = { .op = IR_MUL, .dest = scaled_right, .src1 = right, .src2 = scale_vreg };
            emit(b, mul_ins);
            right = scaled_right;
        }

        int res = new_vreg(b);
        ir_op_t op;
        switch (expr->binop.op) {
            case BINOP_ADD: op = IR_ADD; break;
            case BINOP_SUB: op = IR_SUB; break;
            case BINOP_MUL: op = IR_MUL; break;
            case BINOP_DIV: op = IR_DIV; break;
            case BINOP_MOD: op = IR_MOD; break;
            case BINOP_EQ: op = IR_EQ; break;
            case BINOP_NEQ: op = IR_NEQ; break;
            case BINOP_LT: op = IR_LT; break;
            case BINOP_GT: op = IR_GT; break;
            case BINOP_LTE: op = IR_LTE; break;
            case BINOP_GTE: op = IR_GTE; break;
            case BINOP_BITAND: op = IR_BITAND; break;
            case BINOP_BITOR: op = IR_BITOR; break;
            case BINOP_BITXOR: op = IR_BITXOR; break;
            case BINOP_SHL: op = IR_SHL; break;
            case BINOP_SHR: op = IR_SHR; break;
            default: op = IR_ADD; break;
        }
        ir_instr_t instr = { .op = op, .dest = res, .src1 = left, .src2 = right };
        emit(b, instr);
        return res;
    }

    if (expr->kind == AST_UNOP) {
        int operand = build_expr(b, expr->unop.operand);
        int res = new_vreg(b);
        ir_op_t op = IR_NEG;
        if (expr->unop.op == UNOP_NOT) op = IR_NOT;
        else if (expr->unop.op == UNOP_BITNOT) op = IR_BITNOT;
        ir_instr_t instr = { .op = op, .dest = res, .src1 = operand };
        emit(b, instr);
        return res;
    }

    if (expr->kind == AST_FUNC_CALL) {
        for (size_t i = 0; i < expr->func_call.arg_count; i++) {
            int arg_vreg = build_expr(b, expr->func_call.args[i]);
            ir_instr_t instr = { .op = IR_ARG, .src1 = arg_vreg, .imm = (long long)i };
            emit(b, instr);
        }
        int res = new_vreg(b);
        ir_instr_t instr = {
            .op = IR_CALL,
            .dest = res,
            .imm = (long long)expr->func_call.arg_count,
            .name = expr->func_call.name,
            .name_len = expr->func_call.name_len
        };
        emit(b, instr);
        return res;
    }

    return -1;
}

static void build_stmt(ir_builder_t *b, ast_node_t *stmt) {
    if (!stmt) return;

    if (stmt->kind == AST_BLOCK) {
        for (size_t i = 0; i < stmt->block.stmt_count; i++) {
            build_stmt(b, stmt->block.stmts[i]);
        }
    } else if (stmt->kind == AST_RETURN) {
        int res = stmt->return_stmt.expr ? build_expr(b, stmt->return_stmt.expr) : -1;
        ir_instr_t instr = { .op = IR_RET, .src1 = res };
        emit(b, instr);

        ir_instr_t jmp_epi = { .op = IR_JMP, .imm = b->func->epilogue_label };
        emit(b, jmp_epi);
    } else if (stmt->kind == AST_VAR_DECL) {
        int vreg = stmt->var_decl.symbol_id;
        if (stmt->var_decl.init_expr) {
            int init = build_expr(b, stmt->var_decl.init_expr);
            ir_instr_t instr = { .op = IR_MOV, .dest = vreg, .src1 = init };
            emit(b, instr);
        }
    } else if (stmt->kind == AST_IF) {
        int cond_reg = build_expr(b, stmt->if_stmt.condition);
        int else_label = new_label(b);
        int end_label = new_label(b);
        ir_instr_t jmpz = { .op = IR_JMPZ, .src1 = cond_reg, .imm = else_label };
        emit(b, jmpz);
        build_stmt(b, stmt->if_stmt.then_branch);
        ir_instr_t jmp = { .op = IR_JMP, .imm = end_label };
        emit(b, jmp);
        ir_instr_t lbl_else = { .op = IR_LABEL, .imm = else_label };
        emit(b, lbl_else);
        if (stmt->if_stmt.else_branch) {
            build_stmt(b, stmt->if_stmt.else_branch);
        }
        ir_instr_t lbl_end = { .op = IR_LABEL, .imm = end_label };
        emit(b, lbl_end);
    } else if (stmt->kind == AST_WHILE) {
        int start_label = new_label(b);
        int end_label = new_label(b);

        ir_instr_t lbl_start = { .op = IR_LABEL, .imm = start_label };
        emit(b, lbl_start);

        int cond_reg = build_expr(b, stmt->while_stmt.condition);
        ir_instr_t jmpz = { .op = IR_JMPZ, .src1 = cond_reg, .imm = end_label };
        emit(b, jmpz);

        int old_start = b->loop_start;
        int old_end = b->loop_end;
        b->loop_start = start_label;
        b->loop_end = end_label;

        build_stmt(b, stmt->while_stmt.body);

        b->loop_start = old_start;
        b->loop_end = old_end;

        ir_instr_t jmp = { .op = IR_JMP, .imm = start_label };
        emit(b, jmp);

        ir_instr_t lbl_end = { .op = IR_LABEL, .imm = end_label };
        emit(b, lbl_end);
    } else if (stmt->kind == AST_FOR) {
        if (stmt->for_stmt.init) build_stmt(b, stmt->for_stmt.init);

        int start_label = new_label(b);
        int inc_label = new_label(b);
        int end_label = new_label(b);

        ir_instr_t lbl_start = { .op = IR_LABEL, .imm = start_label };
        emit(b, lbl_start);

        if (stmt->for_stmt.condition) {
            int cond_reg = build_expr(b, stmt->for_stmt.condition);
            ir_instr_t jmpz = { .op = IR_JMPZ, .src1 = cond_reg, .imm = end_label };
            emit(b, jmpz);
        }

        int old_start = b->loop_start;
        int old_end = b->loop_end;
        b->loop_start = inc_label;
        b->loop_end = end_label;

        build_stmt(b, stmt->for_stmt.body);

        b->loop_start = old_start;
        b->loop_end = old_end;

        ir_instr_t lbl_inc = { .op = IR_LABEL, .imm = inc_label };
        emit(b, lbl_inc);

        if (stmt->for_stmt.inc) build_expr(b, stmt->for_stmt.inc);

        ir_instr_t jmp = { .op = IR_JMP, .imm = start_label };
        emit(b, jmp);

        ir_instr_t lbl_end = { .op = IR_LABEL, .imm = end_label };
        emit(b, lbl_end);
    } else if (stmt->kind == AST_BREAK) {
        ir_instr_t jmp = { .op = IR_JMP, .imm = b->loop_end };
        emit(b, jmp);
    } else if (stmt->kind == AST_CONTINUE) {
        ir_instr_t jmp = { .op = IR_JMP, .imm = b->loop_start };
        emit(b, jmp);
    } else {
        build_expr(b, stmt);
    }
}

ir_program_t *ir_build(ast_node_t *prog_ast, arena_t *arena) {
    if (!prog_ast || prog_ast->kind != AST_PROGRAM || !prog_ast->program.funcs) return NULL;

    ir_program_t *prog = arena_alloc(arena, sizeof(ir_program_t));
    if (!prog) return NULL;
    prog->func_count = prog_ast->program.func_count;
    prog->funcs = arena_alloc(arena, prog->func_count * sizeof(ir_func_t *));
    if (!prog->funcs && prog->func_count > 0) return NULL;
    if (prog->funcs) {
        memset(prog->funcs, 0, prog->func_count * sizeof(ir_func_t *));
    }
    prog->strings = NULL;
    prog->string_count = 0;
    prog->string_capacity = 0;

    for (size_t i = 0; i < prog->func_count; i++) {
        ast_node_t *func_ast = prog_ast->program.funcs[i];
        if (!func_ast || func_ast->function.body == NULL) continue;

        ir_func_t *func = arena_alloc(arena, sizeof(ir_func_t));
        if (!func) return NULL;
        func->name = func_ast->function.name;
        func->name_len = func_ast->function.name_len;
        func->instrs = NULL;
        func->instr_count = 0;
        func->instr_capacity = 0;
        func->vreg_count = func_ast->function.max_symbol_id;
        func->param_count = func_ast->function.param_count;

        ir_builder_t b = {
            .func = func,
            .prog = prog,
            .arena = arena,
            .next_label = 0,
            .loop_start = 0,
            .loop_end = 0
        };

        func->epilogue_label = new_label(&b);

        for (size_t j = 0; j < func->param_count; j++) {
            if (func_ast->function.params && func_ast->function.params[j]) {
                ir_instr_t instr = { .op = IR_PARAM, .dest = func_ast->function.params[j]->var_decl.symbol_id, .imm = (long long)j };
                emit(&b, instr);
            }
        }

        build_stmt(&b, func_ast->function.body);

        ir_instr_t lbl_epi = { .op = IR_LABEL, .imm = func->epilogue_label };
        emit(&b, lbl_epi);

        prog->funcs[i] = func;
    }

    return prog;
}

