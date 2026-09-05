#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static type_t type_int_const = { .kind = TYPE_INT, .size = 8, .align = 8 };
static type_t type_char_const = { .kind = TYPE_CHAR, .size = 1, .align = 1 };
static type_t type_void_const = { .kind = TYPE_VOID, .size = 0, .align = 1 };
static type_t type_long_const = { .kind = TYPE_LONG, .size = 8, .align = 8 };

type_t *get_type_int(void) { return &type_int_const; }
type_t *get_type_char(void) { return &type_char_const; }
type_t *get_type_void(void) { return &type_void_const; }
type_t *get_type_long(void) { return &type_long_const; }

type_t *type_make_ptr(arena_t *arena, type_t *base) {
    type_t *t = arena_alloc(arena, sizeof(type_t));
    t->kind = TYPE_PTR;
    t->size = 8;
    t->align = 8;
    t->ptr.base = base;
    return t;
}

void parser_init(parser_t *p, lexer_t *l, arena_t *arena) {
    p->lexer = l;
    p->arena = arena;
    p->error_count = 0;
    p->recursion_depth = 0;
    p->current = lex_next(p->lexer);
}

static int enter_depth(parser_t *p) {
    if (p->recursion_depth >= MAX_PARSER_DEPTH) {
        fprintf(stderr, "Syntax error at %d:%d: Exceeded maximum nesting recursion depth\n", p->current.line, p->current.col);
        p->error_count++;
        return 0;
    }
    p->recursion_depth++;
    return 1;
}

static void leave_depth(parser_t *p) {
    if (p->recursion_depth > 0) {
        p->recursion_depth--;
    }
}

static void advance(parser_t *p) {
    p->current = lex_next(p->lexer);
}

static int expect(parser_t *p, token_kind_t kind) {
    if (p->current.kind == kind) {
        advance(p);
        return 1;
    }
    fprintf(stderr, "Syntax error at %d:%d\n", p->current.line, p->current.col);
    p->error_count++;
    return 0;
}

static int match(parser_t *p, token_kind_t kind) {
    if (p->current.kind == kind) {
        advance(p);
        return 1;
    }
    return 0;
}

static int is_type_token(token_kind_t kind) {
    return kind == TOK_INT_KW || kind == TOK_CHAR_KW || kind == TOK_VOID_KW || kind == TOK_LONG_KW;
}

static type_t *parse_type(parser_t *p) {
    type_t *base = NULL;
    if (match(p, TOK_INT_KW)) base = get_type_int();
    else if (match(p, TOK_CHAR_KW)) base = get_type_char();
    else if (match(p, TOK_VOID_KW)) base = get_type_void();
    else if (match(p, TOK_LONG_KW)) base = get_type_long();
    else return NULL;

    while (match(p, TOK_STAR)) {
        base = type_make_ptr(p->arena, base);
    }
    return base;
}

static ast_node_t *create_node(parser_t *p, ast_node_kind_t kind, int line, int col) {
    ast_node_t *node = arena_alloc(p->arena, sizeof(ast_node_t));
    if (!node) {
        fprintf(stderr, "Fatal error at %d:%d: Out of memory\n", line, col);
        p->error_count++;
        return NULL;
    }
    memset(node, 0, sizeof(ast_node_t));
    node->kind = kind;
    node->line = line;
    node->col = col;
    return node;
}

static ast_node_t *parse_expression(parser_t *p);
static ast_node_t *parse_statement(parser_t *p);
static ast_node_t *parse_unary(parser_t *p);

static ast_node_t *parse_primary(parser_t *p) {
    int line = p->current.line, col = p->current.col;

    if (p->current.kind == TOK_INT_LITERAL) {
        ast_node_t *node = create_node(p, AST_INT_LITERAL, line, col);
        if (!node) return NULL;
        node->int_literal.value = p->current.int_val;
        node->type = get_type_int();
        advance(p);
        return node;
    }

    if (p->current.kind == TOK_CHAR_LITERAL) {
        ast_node_t *node = create_node(p, AST_CHAR_LITERAL, line, col);
        if (!node) return NULL;
        node->char_literal.value = p->current.char_val;
        node->type = get_type_char();
        advance(p);
        return node;
    }

    if (p->current.kind == TOK_STRING) {
        ast_node_t *node = create_node(p, AST_STRING_LITERAL, line, col);
        if (!node) return NULL;
        char *decoded = arena_alloc(p->arena, p->current.length + 1);
        if (!decoded) return NULL;
        size_t dec_len = lex_decode_escape_string(decoded, p->current.start, p->current.length);
        decoded[dec_len] = '\0';

        node->string_literal.val = p->current.start;
        node->string_literal.len = p->current.length;
        node->string_literal.decoded = decoded;
        node->string_literal.decoded_len = dec_len;
        node->type = type_make_ptr(p->arena, get_type_char());
        advance(p);
        return node;
    }

    if (p->current.kind == TOK_SIZEOF_KW) {
        advance(p);
        ast_node_t *node = create_node(p, AST_SIZEOF, line, col);
        if (!node) return NULL;
        node->type = get_type_int();
        if (p->current.kind == TOK_LPAREN) {
            advance(p);
            if (is_type_token(p->current.kind)) {
                node->size_of.target_type = parse_type(p);
                if (!expect(p, TOK_RPAREN)) return NULL;
            } else {
                node->size_of.expr = parse_expression(p);
                if (!expect(p, TOK_RPAREN)) return NULL;
            }
        } else {
            node->size_of.expr = parse_unary(p);
        }
        return node;
    }

    if (p->current.kind == TOK_IDENTIFIER) {
        token_t name_tok = p->current;
        advance(p);

        if (p->current.kind == TOK_LPAREN) {
            advance(p);
            ast_node_t **args = NULL;
            size_t count = 0, capacity = 0;

            if (p->current.kind != TOK_RPAREN) {
                do {
                    ast_node_t *arg = parse_expression(p);
                    if (!arg) return NULL;
                    if (count == capacity) {
                        capacity = capacity == 0 ? 4 : capacity * 2;
                        ast_node_t **new_args = arena_alloc(p->arena, capacity * sizeof(ast_node_t *));
                        if (args) memcpy(new_args, args, count * sizeof(ast_node_t *));
                        args = new_args;
                    }
                    args[count++] = arg;
                } while (match(p, TOK_COMMA));
            }
            if (!expect(p, TOK_RPAREN)) return NULL;

            ast_node_t *node = create_node(p, AST_FUNC_CALL, line, col);
            node->func_call.name = name_tok.start;
            node->func_call.name_len = name_tok.length;
            node->func_call.args = args;
            node->func_call.arg_count = count;
            node->type = get_type_int();
            return node;
        } else {
            ast_node_t *node = create_node(p, AST_VAR, line, col);
            node->var.name = name_tok.start;
            node->var.name_len = name_tok.length;
            return node;
        }
    }

    if (match(p, TOK_LPAREN)) {
        ast_node_t *expr = parse_expression(p);
        if (!expect(p, TOK_RPAREN)) return NULL;
        return expr;
    }

    fprintf(stderr, "Expected primary expression at %d:%d\n", p->current.line, p->current.col);
    p->error_count++;
    return NULL;
}

static ast_node_t *parse_postfix(parser_t *p) {
    ast_node_t *expr = parse_primary(p);
    if (!expr) return NULL;

    while (1) {
        int line = p->current.line, col = p->current.col;
        if (match(p, TOK_LBRACKET)) {
            ast_node_t *idx = parse_expression(p);
            if (!idx || !expect(p, TOK_RBRACKET)) return NULL;

            ast_node_t *add = create_node(p, AST_BINOP, line, col);
            add->binop.op = BINOP_ADD;
            add->binop.left = expr;
            add->binop.right = idx;

            ast_node_t *deref = create_node(p, AST_DEREF, line, col);
            deref->deref.operand = add;
            expr = deref;
        } else {
            break;
        }
    }
    return expr;
}

static ast_node_t *parse_unary(parser_t *p) {
    int line = p->current.line, col = p->current.col;
    if (match(p, TOK_PLUS)) {
        return parse_unary(p);
    }
    if (match(p, TOK_MINUS)) {
        ast_node_t *operand = parse_unary(p);
        if (!operand) return NULL;
        ast_node_t *node = create_node(p, AST_UNOP, line, col);
        node->unop.op = UNOP_NEG;
        node->unop.operand = operand;
        return node;
    }
    if (match(p, TOK_BANG)) {
        ast_node_t *operand = parse_unary(p);
        if (!operand) return NULL;
        ast_node_t *node = create_node(p, AST_UNOP, line, col);
        node->unop.op = UNOP_NOT;
        node->unop.operand = operand;
        return node;
    }
    if (match(p, TOK_TILDE)) {
        ast_node_t *operand = parse_unary(p);
        if (!operand) return NULL;
        ast_node_t *node = create_node(p, AST_UNOP, line, col);
        node->unop.op = UNOP_BITNOT;
        node->unop.operand = operand;
        return node;
    }
    if (match(p, TOK_STAR)) {
        ast_node_t *operand = parse_unary(p);
        if (!operand) return NULL;
        ast_node_t *node = create_node(p, AST_DEREF, line, col);
        node->deref.operand = operand;
        return node;
    }
    if (match(p, TOK_AMP)) {
        ast_node_t *operand = parse_unary(p);
        if (!operand) return NULL;
        ast_node_t *node = create_node(p, AST_ADDR_OF, line, col);
        node->addr_of.operand = operand;
        return node;
    }

    return parse_postfix(p);
}

static ast_node_t *parse_multiplicative(parser_t *p) {
    ast_node_t *left = parse_unary(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_STAR || p->current.kind == TOK_SLASH || p->current.kind == TOK_PERCENT) {
        token_kind_t op_tok = p->current.kind;
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_unary(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = (op_tok == TOK_STAR) ? BINOP_MUL : (op_tok == TOK_SLASH) ? BINOP_DIV : BINOP_MOD;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_additive(parser_t *p) {
    ast_node_t *left = parse_multiplicative(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_PLUS || p->current.kind == TOK_MINUS) {
        token_kind_t op_tok = p->current.kind;
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_multiplicative(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = (op_tok == TOK_PLUS) ? BINOP_ADD : BINOP_SUB;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_shift(parser_t *p) {
    ast_node_t *left = parse_additive(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_SHL || p->current.kind == TOK_SHR) {
        token_kind_t op_tok = p->current.kind;
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_additive(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = (op_tok == TOK_SHL) ? BINOP_SHL : BINOP_SHR;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_relational(parser_t *p) {
    ast_node_t *left = parse_shift(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_LT || p->current.kind == TOK_GT || p->current.kind == TOK_LTE || p->current.kind == TOK_GTE) {
        token_kind_t op_tok = p->current.kind;
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_shift(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = (op_tok == TOK_LT) ? BINOP_LT : (op_tok == TOK_GT) ? BINOP_GT : (op_tok == TOK_LTE) ? BINOP_LTE : BINOP_GTE;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_equality(parser_t *p) {
    ast_node_t *left = parse_relational(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_EQ || p->current.kind == TOK_NEQ) {
        token_kind_t op_tok = p->current.kind;
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_relational(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = (op_tok == TOK_EQ) ? BINOP_EQ : BINOP_NEQ;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_bitwise_and(parser_t *p) {
    ast_node_t *left = parse_equality(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_AMP) {
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_equality(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = BINOP_BITAND;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_bitwise_xor(parser_t *p) {
    ast_node_t *left = parse_bitwise_and(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_CARET) {
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_bitwise_and(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = BINOP_BITXOR;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_bitwise_or(parser_t *p) {
    ast_node_t *left = parse_bitwise_xor(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_PIPE) {
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_bitwise_xor(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = BINOP_BITOR;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_logical_and(parser_t *p) {
    ast_node_t *left = parse_bitwise_or(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_LOGICAL_AND) {
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_bitwise_or(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = BINOP_LOGICAL_AND;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_logical_or(parser_t *p) {
    ast_node_t *left = parse_logical_and(p);
    if (!left) return NULL;

    while (p->current.kind == TOK_LOGICAL_OR) {
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_logical_and(p);
        if (!right) return NULL;

        ast_node_t *node = create_node(p, AST_BINOP, line, col);
        node->binop.op = BINOP_LOGICAL_OR;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

static ast_node_t *parse_assignment(parser_t *p) {
    ast_node_t *left = parse_logical_or(p);
    if (!left) return NULL;

    if (p->current.kind == TOK_ASSIGN || p->current.kind == TOK_PLUS_ASSIGN ||
        p->current.kind == TOK_MINUS_ASSIGN || p->current.kind == TOK_STAR_ASSIGN ||
        p->current.kind == TOK_SLASH_ASSIGN) {
        token_kind_t assign_kind = p->current.kind;
        int line = p->current.line, col = p->current.col;
        advance(p);
        ast_node_t *right = parse_assignment(p);
        if (!right) return NULL;

        if (assign_kind != TOK_ASSIGN) {
            ast_node_t *op_node = create_node(p, AST_BINOP, line, col);
            if (assign_kind == TOK_PLUS_ASSIGN) op_node->binop.op = BINOP_ADD;
            else if (assign_kind == TOK_MINUS_ASSIGN) op_node->binop.op = BINOP_SUB;
            else if (assign_kind == TOK_STAR_ASSIGN) op_node->binop.op = BINOP_MUL;
            else op_node->binop.op = BINOP_DIV;
            op_node->binop.left = left;
            op_node->binop.right = right;
            right = op_node;
        }

        ast_node_t *node = create_node(p, AST_ASSIGN, line, col);
        node->assign.target = left;
        if (left->kind == AST_VAR) {
            node->assign.name = left->var.name;
            node->assign.name_len = left->var.name_len;
        }
        node->assign.expr = right;
        return node;
    }
    return left;
}

static ast_node_t *parse_statement_body(parser_t *p);

static ast_node_t *parse_expression(parser_t *p) {
    if (!enter_depth(p)) return NULL;
    ast_node_t *res = parse_assignment(p);
    leave_depth(p);
    return res;
}

static ast_node_t *parse_block(parser_t *p) {
    int line = p->current.line, col = p->current.col;
    if (!expect(p, TOK_LBRACE)) return NULL;

    ast_node_t **stmts = NULL;
    size_t count = 0;
    size_t capacity = 0;

    while (p->current.kind != TOK_RBRACE && p->current.kind != TOK_EOF) {
        ast_node_t *stmt = parse_statement(p);
        if (!stmt) return NULL;

        if (count == capacity) {
            capacity = capacity == 0 ? 8 : capacity * 2;
            ast_node_t **new_stmts = arena_alloc(p->arena, capacity * sizeof(ast_node_t *));
            if (!new_stmts) return NULL;
            if (stmts) {
                memcpy(new_stmts, stmts, count * sizeof(ast_node_t *));
            }
            stmts = new_stmts;
        }
        stmts[count++] = stmt;
    }

    if (!expect(p, TOK_RBRACE)) return NULL;

    ast_node_t *node = create_node(p, AST_BLOCK, line, col);
    if (!node) return NULL;
    node->block.stmts = stmts;
    node->block.stmt_count = count;
    return node;
}

static ast_node_t *parse_statement(parser_t *p) {
    if (!enter_depth(p)) return NULL;
    ast_node_t *res = parse_statement_body(p);
    leave_depth(p);
    return res;
}

static ast_node_t *parse_statement_body(parser_t *p) {
    int line = p->current.line, col = p->current.col;
    if (p->current.kind == TOK_RETURN_KW) {
        advance(p);
        ast_node_t *expr = NULL;
        if (p->current.kind != TOK_SEMI) {
            expr = parse_expression(p);
            if (!expr) return NULL;
        }
        if (!expect(p, TOK_SEMI)) return NULL;

        ast_node_t *node = create_node(p, AST_RETURN, line, col);
        if (!node) return NULL;
        node->return_stmt.expr = expr;
        return node;
    }

    if (is_type_token(p->current.kind)) {
        type_t *var_type = parse_type(p);
        if (!var_type) return NULL;

        if (p->current.kind != TOK_IDENTIFIER) {
            fprintf(stderr, "Expected identifier after type at %d:%d\n", p->current.line, p->current.col);
            p->error_count++;
            return NULL;
        }
        token_t name_tok = p->current;
        advance(p);

        ast_node_t *init_expr = NULL;
        if (match(p, TOK_ASSIGN)) {
            init_expr = parse_expression(p);
            if (!init_expr) return NULL;
        }

        if (!expect(p, TOK_SEMI)) return NULL;

        ast_node_t *node = create_node(p, AST_VAR_DECL, line, col);
        if (!node) return NULL;
        node->var_decl.name = name_tok.start;
        node->var_decl.name_len = name_tok.length;
        node->var_decl.type_spec = var_type;
        node->var_decl.init_expr = init_expr;
        node->type = var_type;
        return node;
    }

    if (p->current.kind == TOK_LBRACE) {
        return parse_block(p);
    }

    if (p->current.kind == TOK_SEMI) {
        advance(p);
        ast_node_t *node = create_node(p, AST_BLOCK, line, col);
        if (!node) return NULL;
        node->block.stmts = NULL;
        node->block.stmt_count = 0;
        return node;
    }

    if (p->current.kind == TOK_IF_KW) {
        advance(p);
        if (!expect(p, TOK_LPAREN)) return NULL;
        ast_node_t *cond = parse_expression(p);
        if (!cond) return NULL;
        if (!expect(p, TOK_RPAREN)) return NULL;
        ast_node_t *then_branch = parse_statement(p);
        if (!then_branch) return NULL;
        ast_node_t *else_branch = NULL;
        if (match(p, TOK_ELSE_KW)) {
            else_branch = parse_statement(p);
            if (!else_branch) return NULL;
        }
        ast_node_t *node = create_node(p, AST_IF, line, col);
        if (!node) return NULL;
        node->if_stmt.condition = cond;
        node->if_stmt.then_branch = then_branch;
        node->if_stmt.else_branch = else_branch;
        return node;
    }

    if (p->current.kind == TOK_WHILE_KW) {
        advance(p);
        if (!expect(p, TOK_LPAREN)) return NULL;
        ast_node_t *cond = parse_expression(p);
        if (!cond) return NULL;
        if (!expect(p, TOK_RPAREN)) return NULL;
        ast_node_t *body = parse_statement(p);
        if (!body) return NULL;
        ast_node_t *node = create_node(p, AST_WHILE, line, col);
        if (!node) return NULL;
        node->while_stmt.condition = cond;
        node->while_stmt.body = body;
        return node;
    }

    if (p->current.kind == TOK_FOR_KW) {
        advance(p);
        if (!expect(p, TOK_LPAREN)) return NULL;

        ast_node_t *init = NULL;
        if (p->current.kind != TOK_SEMI) {
            init = parse_statement(p);
            if (!init) return NULL;
        } else {
            advance(p);
        }

        ast_node_t *cond = NULL;
        if (p->current.kind != TOK_SEMI) {
            cond = parse_expression(p);
            if (!cond) return NULL;
        }
        if (!expect(p, TOK_SEMI)) return NULL;

        ast_node_t *inc = NULL;
        if (p->current.kind != TOK_RPAREN) {
            inc = parse_expression(p);
            if (!inc) return NULL;
        }
        if (!expect(p, TOK_RPAREN)) return NULL;

        ast_node_t *body = parse_statement(p);
        if (!body) return NULL;

        ast_node_t *node = create_node(p, AST_FOR, line, col);
        if (!node) return NULL;
        node->for_stmt.init = init;
        node->for_stmt.condition = cond;
        node->for_stmt.inc = inc;
        node->for_stmt.body = body;
        return node;
    }

    if (p->current.kind == TOK_BREAK_KW) {
        advance(p);
        if (!expect(p, TOK_SEMI)) return NULL;
        return create_node(p, AST_BREAK, line, col);
    }

    if (p->current.kind == TOK_CONTINUE_KW) {
        advance(p);
        if (!expect(p, TOK_SEMI)) return NULL;
        return create_node(p, AST_CONTINUE, line, col);
    }

    ast_node_t *expr = parse_expression(p);
    if (!expr) return NULL;
    if (!expect(p, TOK_SEMI)) return NULL;

    return expr;
}

static ast_node_t *parse_function(parser_t *p) {
    int line = p->current.line, col = p->current.col;
    type_t *ret_type = parse_type(p);
    if (!ret_type) return NULL;

    if (p->current.kind != TOK_IDENTIFIER) {
        fprintf(stderr, "Expected function name identifier at %d:%d\n", p->current.line, p->current.col);
        p->error_count++;
        return NULL;
    }
    token_t name_tok = p->current;
    advance(p);

    if (!expect(p, TOK_LPAREN)) return NULL;

    ast_node_t **params = NULL;
    size_t count = 0, capacity = 0;
    int is_varargs = 0;

    if (p->current.kind != TOK_RPAREN) {
        do {
            int p_line = p->current.line, p_col = p->current.col;
            type_t *param_type = parse_type(p);
            if (!param_type) return NULL;

            token_t param_tok = {0};
            if (p->current.kind == TOK_IDENTIFIER) {
                param_tok = p->current;
                advance(p);
            }

            ast_node_t *param = create_node(p, AST_VAR_DECL, p_line, p_col);
            if (!param) return NULL;
            param->var_decl.name = param_tok.start;
            param->var_decl.name_len = param_tok.length;
            param->var_decl.type_spec = param_type;
            param->var_decl.init_expr = NULL;
            param->type = param_type;

            if (count == capacity) {
                capacity = capacity == 0 ? 4 : capacity * 2;
                ast_node_t **new_params = arena_alloc(p->arena, capacity * sizeof(ast_node_t *));
                if (!new_params) return NULL;
                if (params) memcpy(new_params, params, count * sizeof(ast_node_t *));
                params = new_params;
            }
            params[count++] = param;
        } while (match(p, TOK_COMMA));
    }

    if (!expect(p, TOK_RPAREN)) return NULL;

    ast_node_t *body = NULL;
    if (match(p, TOK_SEMI)) {
        if (count == 0) is_varargs = 1;
    } else {
        body = parse_block(p);
        if (!body) return NULL;
    }

    ast_node_t *node = create_node(p, AST_FUNCTION, line, col);
    if (!node) return NULL;
    node->function.name = name_tok.start;
    node->function.name_len = name_tok.length;
    node->function.ret_type = ret_type;
    node->function.params = params;
    node->function.param_count = count;
    node->function.body = body;
    node->function.is_varargs = is_varargs;

    return node;
}

ast_node_t *parse_translation_unit(parser_t *p) {
    int line = p->current.line, col = p->current.col;

    ast_node_t **funcs = NULL;
    size_t count = 0, capacity = 0;

    while (p->current.kind != TOK_EOF) {
        ast_node_t *func = parse_function(p);
        if (!func) return NULL;

        if (count == capacity) {
            capacity = capacity == 0 ? 4 : capacity * 2;
            ast_node_t **new_funcs = arena_alloc(p->arena, capacity * sizeof(ast_node_t *));
            if (!new_funcs) return NULL;
            if (funcs) memcpy(new_funcs, funcs, count * sizeof(ast_node_t *));
            funcs = new_funcs;
        }
        funcs[count++] = func;
    }

    ast_node_t *node = create_node(p, AST_PROGRAM, line, col);
    if (!node) return NULL;
    node->program.funcs = funcs;
    node->program.func_count = count;

    return node;
}

