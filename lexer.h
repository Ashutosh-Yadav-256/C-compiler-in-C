#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum {
    TOK_EOF,
    TOK_ERROR,

    TOK_INT_KW,
    TOK_CHAR_KW,
    TOK_VOID_KW,
    TOK_LONG_KW,
    TOK_RETURN_KW,
    TOK_IF_KW,
    TOK_ELSE_KW,
    TOK_WHILE_KW,
    TOK_FOR_KW,
    TOK_BREAK_KW,
    TOK_CONTINUE_KW,
    TOK_SIZEOF_KW,

    TOK_IDENTIFIER,
    TOK_INT_LITERAL,
    TOK_CHAR_LITERAL,
    TOK_STRING,

    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_SEMI,
    TOK_COMMA,

    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_ASSIGN,
    TOK_PLUS_ASSIGN,
    TOK_MINUS_ASSIGN,
    TOK_STAR_ASSIGN,
    TOK_SLASH_ASSIGN,
    TOK_INC,
    TOK_DEC,
    TOK_EQ,
    TOK_NEQ,
    TOK_LT,
    TOK_GT,
    TOK_LTE,
    TOK_GTE,
    TOK_BANG,
    TOK_TILDE,
    TOK_AMP,
    TOK_PIPE,
    TOK_CARET,
    TOK_SHL,
    TOK_SHR,
    TOK_LOGICAL_AND,
    TOK_LOGICAL_OR,
    TOK_ARROW,
    TOK_DOT
} token_kind_t;

typedef struct {
    token_kind_t kind;
    const char *start;
    size_t length;
    long long int_val;
    char char_val;
    int line;
    int col;
} token_t;

typedef struct {
    const char *source;
    size_t pos;
    int line;
    int col;
} lexer_t;

void lex_init(lexer_t *l, const char *source);
token_t lex_next(lexer_t *l);
size_t lex_decode_escape_string(char *dest, const char *src, size_t src_len);

#endif

