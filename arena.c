#include "arena.h"
#include <stdlib.h>
#include <stdint.h>

typedef struct arena_chunk {
    struct arena_chunk *next;
    size_t capacity;
    size_t offset;
} arena_chunk_t;

struct arena {
    arena_chunk_t *head;
    arena_chunk_t *current;
    size_t default_chunk_size;
};

#include <limits.h>

static arena_chunk_t *create_chunk(size_t capacity) {
    if (capacity > SIZE_MAX - sizeof(arena_chunk_t)) return NULL;
    arena_chunk_t *chunk = malloc(sizeof(arena_chunk_t) + capacity);
    if (!chunk) return NULL;
    chunk->next = NULL;
    chunk->capacity = capacity;
    chunk->offset = 0;
    return chunk;
}

arena_t *arena_create(size_t initial_size) {
    arena_t *arena = malloc(sizeof(arena_t));
    if (!arena) return NULL;

    size_t default_size = initial_size > 0 ? initial_size : (64 * 1024);
    if (default_size > SIZE_MAX - sizeof(arena_chunk_t)) {
        free(arena);
        return NULL;
    }

    arena->head = create_chunk(default_size);
    if (!arena->head) {
        free(arena);
        return NULL;
    }

    arena->current = arena->head;
    arena->default_chunk_size = default_size;
    return arena;
}

void *arena_alloc_aligned(arena_t *arena, size_t size, size_t align) {
    if (!arena || !arena->current || size == 0) return NULL;
    if (align == 0 || (align & (align - 1)) != 0) align = 8;
    if (size > SIZE_MAX - align) return NULL;

    arena_chunk_t *chunk = arena->current;
    uint8_t *base = (uint8_t *)(chunk + 1);
    uintptr_t current_addr = (uintptr_t)(base + chunk->offset);
    size_t padding = (align - (current_addr % align)) % align;

    if (chunk->offset <= chunk->capacity &&
        padding <= chunk->capacity - chunk->offset &&
        size <= chunk->capacity - chunk->offset - padding) {
        chunk->offset += padding;
        void *ptr = base + chunk->offset;
        chunk->offset += size;
        return ptr;
    }

    size_t new_cap = (size + align > arena->default_chunk_size) ? (size + align) : arena->default_chunk_size;
    if (new_cap > SIZE_MAX - sizeof(arena_chunk_t)) return NULL;

    arena_chunk_t *new_chunk = create_chunk(new_cap);
    if (!new_chunk) return NULL;

    arena->current->next = new_chunk;
    arena->current = new_chunk;

    base = (uint8_t *)(new_chunk + 1);
    current_addr = (uintptr_t)base;
    padding = (align - (current_addr % align)) % align;
    new_chunk->offset = padding + size;
    return base + padding;
}

void *arena_alloc(arena_t *arena, size_t size) {
    return arena_alloc_aligned(arena, size, 8);
}

arena_mark_t arena_save(arena_t *arena) {
    arena_mark_t mark = {
        .chunk = arena ? arena->current : NULL,
        .offset = (arena && arena->current) ? arena->current->offset : 0
    };
    return mark;
}

void arena_restore(arena_t *arena, arena_mark_t mark) {
    if (!arena || !mark.chunk) return;
    arena_chunk_t *target = (arena_chunk_t *)mark.chunk;
    target->offset = mark.offset;
    arena->current = target;
}

void arena_reset(arena_t *arena) {
    if (!arena) return;
    for (arena_chunk_t *curr = arena->head; curr != NULL; curr = curr->next) {
        curr->offset = 0;
    }
    arena->current = arena->head;
}

void arena_destroy(arena_t *arena) {
    if (!arena) return;
    arena_chunk_t *curr = arena->head;
    while (curr) {
        arena_chunk_t *next = curr->next;
        free(curr);
        curr = next;
    }
    free(arena);
}

size_t arena_used(const arena_t *arena) {
    if (!arena) return 0;
    size_t total = 0;
    for (arena_chunk_t *curr = arena->head; curr != NULL; curr = curr->next) {
        total += curr->offset;
        if (curr == arena->current) break;
    }
    return total;
}

size_t arena_remaining(const arena_t *arena) {
    if (!arena || !arena->current) return 0;
    return arena->current->capacity - arena->current->offset;
}

