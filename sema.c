#include "sema.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

#define MAX_VARS 1024
#define MAX_SCOPES 128
#define MAX_FUNCS 256

typedef struct {
    const char *name;
    size_t name_len;
    type_t *type;
    int symbol_id;
    int scope_depth;
} symbol_t;

typedef struct {
    const char *name;
    size_t name_len;
    type_t *ret_type;
    type_t **param_types;
    size_t param_count;
    int is_varargs;
    int is_defined;
} func_sym_t;

typedef struct {
    symbol_t vars[MAX_VARS];
    int var_count;
    int scope_depth;
    int loop_depth;

    func_sym_t funcs[MAX_FUNCS];
    int func_count;

    type_t *current_func_ret_type;
    arena_t *arena;
    int error_count;
} sema_context_t;

static int find_var_all(sema_context_t *ctx, const char *name, size_t name_len) {
    if (!name) return -1;
    for (int i = ctx->var_count - 1; i >= 0; i--) {
        if (ctx->vars[i].name && ctx->vars[i].name_len == name_len &&
            strncmp(ctx->vars[i].name, name, name_len) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_var_current_scope(sema_context_t *ctx, const char *name, size_t name_len) {
    if (!name) return -1;
    for (int i = ctx->var_count - 1; i >= 0; i--) {
        if (ctx->vars[i].scope_depth < ctx->scope_depth) break;
        if (ctx->vars[i].name && ctx->vars[i].name_len == name_len &&
            strncmp(ctx->vars[i].name, name, name_len) == 0) {
            return i;
        }
    }
    return -1;
}

static func_sym_t *find_func(sema_context_t *ctx, const char *name, size_t name_len) {
    if (!name) return -1;
    for (int i = 0; i < ctx->func_count; i++) {
        if (ctx->funcs[i].name && ctx->funcs[i].name_len == name_len &&
            strncmp(ctx->funcs[i].name, name, name_len) == 0) {
            return &ctx->funcs[i];
        }
    }
    return NULL;
}

static void check_node(sema_context_t *ctx, ast_node_t *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM: {
            if (node->program.funcs) {
                for (size_t i = 0; i < node->program.func_count; i++) {
                    ast_node_t *f = node->program.funcs[i];
                    if (!f) continue;
                    func_sym_t *existing = find_func(ctx, f->function.name, f->function.name_len);
                    if (existing) {
                        if (existing->is_defined && f->function.body != NULL) {
                            fprintf(stderr, "Error at %d:%d: Redefinition of function '%.*s'\n",
                                    f->line, f->col, (int)f->function.name_len, f->function.name ? f->function.name : "");
                            ctx->error_count++;
                        }
                    } else {
                        if (ctx->func_count >= MAX_FUNCS) {
                            fprintf(stderr, "Error at %d:%d: Too many functions\n", f->line, f->col);
                            ctx->error_count++;
                            break;
                        }
                        func_sym_t *fn = &ctx->funcs[ctx->func_count++];
                        fn->name = f->function.name;
                        fn->name_len = f->function.name_len;
                        fn->ret_type = f->function.ret_type ? f->function.ret_type : get_type_int();
                        fn->param_count = f->function.param_count;
                        fn->is_varargs = f->function.is_varargs;
                        fn->is_defined = (f->function.body != NULL);
                    }
                }
                for (size_t i = 0; i < node->program.func_count; i++) {
                    if (node->program.funcs[i] && node->program.funcs[i]->function.body != NULL) {
                        check_node(ctx, node->program.funcs[i]);
                    }
                }
            }
            break;
        }
        case AST_FUNCTION: {
            if (node->function.body == NULL) break;

            ctx->var_count = 0;
            ctx->scope_depth = 1;
            ctx->current_func_ret_type = node->function.ret_type ? node->function.ret_type : get_type_int();

            for (size_t i = 0; i < node->function.param_count; i++) {
                if (!node->function.params) break;
                ast_node_t *param = node->function.params[i];
                if (!param) continue;
                if (param->var_decl.name && find_var_current_scope(ctx, param->var_decl.name, param->var_decl.name_len) != -1) {
                    fprintf(stderr, "Error at %d:%d: Duplicate parameter '%.*s'\n",
                            param->line, param->col, (int)param->var_decl.name_len, param->var_decl.name);
                    ctx->error_count++;
                }
                if (ctx->var_count >= MAX_VARS) {
                    fprintf(stderr, "Error at %d:%d: Too many variables\n", param->line, param->col);
                    ctx->error_count++;
                    break;
                }
                ctx->vars[ctx->var_count].name = param->var_decl.name;
                ctx->vars[ctx->var_count].name_len = param->var_decl.name_len;
                ctx->vars[ctx->var_count].type = param->var_decl.type_spec ? param->var_decl.type_spec : get_type_int();
                ctx->vars[ctx->var_count].scope_depth = ctx->scope_depth;
                ctx->vars[ctx->var_count].symbol_id = ctx->var_count;
                param->var_decl.symbol_id = ctx->var_count;
                param->type = ctx->vars[ctx->var_count].type;
                ctx->var_count++;
            }

            check_node(ctx, node->function.body);
            node->function.max_symbol_id = ctx->var_count;
            break;
        }
        case AST_BLOCK: {
            if (ctx->scope_depth >= MAX_SCOPES) {
                fprintf(stderr, "Error at %d:%d: Exceeded maximum scope nesting depth\n", node->line, node->col);
                ctx->error_count++;
                return;
            }
            ctx->scope_depth++;
            int old_count = ctx->var_count;
            if (node->block.stmts) {
                for (size_t i = 0; i < node->block.stmt_count; i++) {
                    check_node(ctx, node->block.stmts[i]);
                }
            }
            ctx->var_count = old_count;
            ctx->scope_depth--;
            break;
        }
        case AST_VAR_DECL: {
            if (node->var_decl.init_expr) {
                check_node(ctx, node->var_decl.init_expr);
            }
            if (find_var_current_scope(ctx, node->var_decl.name, node->var_decl.name_len) != -1) {
                fprintf(stderr, "Error at %d:%d: Duplicate declaration of variable '%.*s' in current scope\n",
                        node->line, node->col, (int)node->var_decl.name_len, node->var_decl.name);
                ctx->error_count++;
            }
            if (ctx->var_count >= MAX_VARS) {
                fprintf(stderr, "Error at %d:%d: Too many variables\n", node->line, node->col);
                ctx->error_count++;
                return;
            }
            type_t *vtype = node->var_decl.type_spec ? node->var_decl.type_spec : get_type_int();
            ctx->vars[ctx->var_count].name = node->var_decl.name;
            ctx->vars[ctx->var_count].name_len = node->var_decl.name_len;
            ctx->vars[ctx->var_count].type = vtype;
            ctx->vars[ctx->var_count].scope_depth = ctx->scope_depth;
            ctx->vars[ctx->var_count].symbol_id = ctx->var_count;
            node->var_decl.symbol_id = ctx->var_count;
            node->type = vtype;
            ctx->var_count++;
            break;
        }
        case AST_VAR: {
            int idx = find_var_all(ctx, node->var.name, node->var.name_len);
            if (idx == -1) {
                fprintf(stderr, "Error at %d:%d: Undeclared variable '%.*s'\n",
                        node->line, node->col, (int)node->var.name_len, node->var.name);
                ctx->error_count++;
                node->type = get_type_int();
            } else {
                node->var.symbol_id = ctx->vars[idx].symbol_id;
                node->type = ctx->vars[idx].type;
            }
            break;
        }
        case AST_ASSIGN: {
            if (node->assign.target) {
                check_node(ctx, node->assign.target);
                node->type = node->assign.target->type;
                if (node->assign.target->kind == AST_VAR) {
                    node->assign.symbol_id = node->assign.target->var.symbol_id;
                }
            } else if (node->assign.name) {
                int idx = find_var_all(ctx, node->assign.name, node->assign.name_len);
                if (idx == -1) {
                    fprintf(stderr, "Error at %d:%d: Undeclared variable '%.*s'\n",
                            node->line, node->col, (int)node->assign.name_len, node->assign.name);
                    ctx->error_count++;
                    node->type = get_type_int();
                } else {
                    node->assign.symbol_id = ctx->vars[idx].symbol_id;
                    node->type = ctx->vars[idx].type;
                }
            }
            check_node(ctx, node->assign.expr);
            break;
        }
        case AST_BINOP: {
            check_node(ctx, node->binop.left);
            check_node(ctx, node->binop.right);
            if (node->binop.left && node->binop.left->type && node->binop.left->type->kind == TYPE_PTR) {
                node->type = node->binop.left->type;
            } else if (node->binop.right && node->binop.right->type && node->binop.right->type->kind == TYPE_PTR) {
                node->type = node->binop.right->type;
            } else {
                node->type = get_type_int();
            }
            break;
        }
        case AST_UNOP: {
            check_node(ctx, node->unop.operand);
            node->type = node->unop.operand ? node->unop.operand->type : get_type_int();
            break;
        }
        case AST_ADDR_OF: {
            check_node(ctx, node->addr_of.operand);
            type_t *base = node->addr_of.operand ? node->addr_of.operand->type : get_type_int();
            node->type = type_make_ptr(ctx->arena, base);
            break;
        }
        case AST_DEREF: {
            check_node(ctx, node->deref.operand);
            if (node->deref.operand && node->deref.operand->type && node->deref.operand->type->kind == TYPE_PTR) {
                node->type = node->deref.operand->type->ptr.base;
            } else {
                node->type = get_type_int();
            }
            break;
        }
        case AST_SIZEOF: {
            if (node->size_of.expr) check_node(ctx, node->size_of.expr);
            node->type = get_type_int();
            break;
        }
        case AST_RETURN: {
            if (node->return_stmt.expr) {
                check_node(ctx, node->return_stmt.expr);
            }
            break;
        }
        case AST_IF: {
            check_node(ctx, node->if_stmt.condition);
            check_node(ctx, node->if_stmt.then_branch);
            if (node->if_stmt.else_branch) {
                check_node(ctx, node->if_stmt.else_branch);
            }
            break;
        }
        case AST_WHILE: {
            check_node(ctx, node->while_stmt.condition);
            ctx->loop_depth++;
            check_node(ctx, node->while_stmt.body);
            ctx->loop_depth--;
            break;
        }
        case AST_FOR: {
            if (node->for_stmt.init) check_node(ctx, node->for_stmt.init);
            if (node->for_stmt.condition) check_node(ctx, node->for_stmt.condition);
            if (node->for_stmt.inc) check_node(ctx, node->for_stmt.inc);
            ctx->loop_depth++;
            check_node(ctx, node->for_stmt.body);
            ctx->loop_depth--;
            break;
        }
        case AST_BREAK: {
            if (ctx->loop_depth == 0) {
                fprintf(stderr, "Error at %d:%d: 'break' outside of loop\n", node->line, node->col);
                ctx->error_count++;
            }
            break;
        }
        case AST_CONTINUE: {
            if (ctx->loop_depth == 0) {
                fprintf(stderr, "Error at %d:%d: 'continue' outside of loop\n", node->line, node->col);
                ctx->error_count++;
            }
            break;
        }
        case AST_FUNC_CALL: {
            func_sym_t *fn = find_func(ctx, node->func_call.name, node->func_call.name_len);
            if (!fn) {
                fprintf(stderr, "Error at %d:%d: Call to undeclared function '%.*s'\n",
                        node->line, node->col, (int)node->func_call.name_len, node->func_call.name);
                ctx->error_count++;
                node->type = get_type_int();
            } else {
                if (!fn->is_varargs && fn->param_count != node->func_call.arg_count) {
                    fprintf(stderr, "Error at %d:%d: Function '%.*s' expects %d arguments, got %d\n",
                            node->line, node->col, (int)node->func_call.name_len, node->func_call.name,
                            (int)fn->param_count, (int)node->func_call.arg_count);
                    ctx->error_count++;
                }
                node->type = fn->ret_type ? fn->ret_type : get_type_int();
            }
            for (size_t i = 0; i < node->func_call.arg_count; i++) {
                check_node(ctx, node->func_call.args[i]);
            }
            break;
        }
        case AST_INT_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_STRING_LITERAL:
        case AST_CAST:
            break;
    }
}

sema_result_t sema_check(ast_node_t *root, arena_t *arena) {
    sema_context_t ctx = {0};
    ctx.arena = arena;
    check_node(&ctx, root);
    sema_result_t res = { .error_count = ctx.error_count };
    return res;
}

