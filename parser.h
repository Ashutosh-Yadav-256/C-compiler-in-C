#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "arena.h"

#define MAX_PARSER_DEPTH 500

typedef struct {
    lexer_t *lexer;
    arena_t *arena;
    token_t current;
    int error_count;
    int recursion_depth;
} parser_t;

void parser_init(parser_t *p, lexer_t *l, arena_t *arena);
ast_node_t *parse_translation_unit(parser_t *p);

type_t *get_type_int(void);
type_t *get_type_char(void);
type_t *get_type_void(void);
type_t *get_type_long(void);
type_t *type_make_ptr(arena_t *arena, type_t *base);

#endif

