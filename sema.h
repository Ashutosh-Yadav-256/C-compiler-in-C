#ifndef SEMA_H
#define SEMA_H

#include "ast.h"
#include "arena.h"

typedef struct {
    int error_count;
} sema_result_t;

sema_result_t sema_check(ast_node_t *root, arena_t *arena);

#endif

