#ifndef AST_H
#define AST_H

#include <stddef.h>

typedef enum {
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_INT,
    TYPE_LONG,
    TYPE_PTR,
    TYPE_ARRAY,
    TYPE_FUNC
} type_kind_t;

typedef struct type type_t;

struct type {
    type_kind_t kind;
    size_t size;
    size_t align;
    union {
        struct {
            type_t *base;
        } ptr;
        struct {
            type_t *elem;
            size_t length;
        } array;
        struct {
            type_t *ret_type;
            type_t **param_types;
            size_t param_count;
            int is_varargs;
        } func;
    };
};

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_BLOCK,
    AST_RETURN,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_BINOP,
    AST_UNOP,
    AST_INT_LITERAL,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
    AST_VAR,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_BREAK,
    AST_CONTINUE,
    AST_FUNC_CALL,
    AST_ADDR_OF,
    AST_DEREF,
    AST_SIZEOF,
    AST_CAST
} ast_node_kind_t;

typedef enum {
    BINOP_ADD, BINOP_SUB, BINOP_MUL, BINOP_DIV, BINOP_MOD,
    BINOP_EQ, BINOP_NEQ, BINOP_LT, BINOP_GT, BINOP_LTE, BINOP_GTE,
    BINOP_BITAND, BINOP_BITOR, BINOP_BITXOR,
    BINOP_SHL, BINOP_SHR,
    BINOP_LOGICAL_AND, BINOP_LOGICAL_OR
} binop_kind_t;

typedef enum {
    UNOP_NEG, UNOP_NOT, UNOP_BITNOT, UNOP_ADDR_OF, UNOP_DEREF
} unop_kind_t;

typedef struct ast_node {
    ast_node_kind_t kind;
    int line;
    int col;
    type_t *type;
    union {
        struct {
            struct ast_node **funcs;
            size_t func_count;
        } program;
        struct {
            const char *name;
            size_t name_len;
            type_t *ret_type;
            struct ast_node **params;
            size_t param_count;
            struct ast_node *body;
            int max_symbol_id;
            int is_varargs;
        } function;
        struct {
            struct ast_node **stmts;
            size_t stmt_count;
        } block;
        struct {
            struct ast_node *expr;
        } return_stmt;
        struct {
            const char *name;
            size_t name_len;
            type_t *type_spec;
            struct ast_node *init_expr;
            int symbol_id;
        } var_decl;
        struct {
            const char *name;
            size_t name_len;
            struct ast_node *target;
            struct ast_node *expr;
            int symbol_id;
        } assign;
        struct {
            binop_kind_t op;
            struct ast_node *left;
            struct ast_node *right;
        } binop;
        struct {
            unop_kind_t op;
            struct ast_node *operand;
        } unop;
        struct {
            long long value;
        } int_literal;
        struct {
            const char *val;
            size_t len;
            const char *decoded;
            size_t decoded_len;
        } string_literal;
        struct {
            char value;
        } char_literal;
        struct {
            const char *name;
            size_t name_len;
            int symbol_id;
        } var;
        struct {
            struct ast_node *condition;
            struct ast_node *then_branch;
            struct ast_node *else_branch;
        } if_stmt;
        struct {
            struct ast_node *condition;
            struct ast_node *body;
        } while_stmt;
        struct {
            struct ast_node *init;
            struct ast_node *condition;
            struct ast_node *inc;
            struct ast_node *body;
        } for_stmt;
        struct {
            const char *name;
            size_t name_len;
            struct ast_node **args;
            size_t arg_count;
        } func_call;
        struct {
            struct ast_node *operand;
        } addr_of;
        struct {
            struct ast_node *operand;
        } deref;
        struct {
            type_t *target_type;
            struct ast_node *expr;
        } cast;
        struct {
            type_t *target_type;
            struct ast_node *expr;
        } size_of;
    };
} ast_node_t;

#endif

