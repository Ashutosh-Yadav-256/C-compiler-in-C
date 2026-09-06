#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct arena arena_t;

typedef struct {
    void *chunk;
    size_t offset;
} arena_mark_t;

arena_t *arena_create(size_t initial_size);
void *arena_alloc_aligned(arena_t *arena, size_t size, size_t align);
void *arena_alloc(arena_t *arena, size_t size);
arena_mark_t arena_save(arena_t *arena);
void arena_restore(arena_t *arena, arena_mark_t mark);
void arena_reset(arena_t *arena);
void arena_destroy(arena_t *arena);
size_t arena_used(const arena_t *arena);
size_t arena_remaining(const arena_t *arena);

#endif

