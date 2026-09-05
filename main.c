#include <stdio.h>
#include <stdlib.h>
#include "arena.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "sema.h"
#include "ir.h"
#include "opt.h"

#define MAX_SOURCE_SIZE (64 * 1024 * 1024)

static char *read_file(const char *path) {
    if (!path) return NULL;

    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        fprintf(stderr, "Could not determine file size for \"%s\".\n", path);
        fclose(file);
        return NULL;
    }

    long tell_size = ftell(file);
    if (tell_size < 0) {
        fprintf(stderr, "Error reading file size for \"%s\".\n", path);
        fclose(file);
        return NULL;
    }
    if ((size_t)tell_size > MAX_SOURCE_SIZE) {
        fprintf(stderr, "File \"%s\" exceeds maximum supported source size (64MB).\n", path);
        fclose(file);
        return NULL;
    }
    size_t file_size = (size_t)tell_size;
    rewind(file);

    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size && ferror(file)) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.c> <output.asm>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];

    char *source = read_file(input_path);
    if (!source) {
        return 1;
    }

    arena_t *arena = arena_create(64 * 1024);
    if (!arena) {
        fprintf(stderr, "Failed to create arena\n");
        free(source);
        return 1;
    }

    lexer_t lexer;
    lex_init(&lexer, source);

    parser_t parser;
    parser_init(&parser, &lexer, arena);

    ast_node_t *ast = parse_translation_unit(&parser);
    if (!ast || parser.error_count > 0) {
        fprintf(stderr, "Parsing failed with %d error(s)\n", parser.error_count);
        arena_destroy(arena);
        free(source);
        return 1;
    }

    sema_result_t sema_res = sema_check(ast, arena);
    if (sema_res.error_count > 0) {
        fprintf(stderr, "Semantic analysis failed with %d errors\n", sema_res.error_count);
        arena_destroy(arena);
        free(source);
        return 1;
    }

    ir_program_t *prog = ir_build(ast, arena);
    if (!prog) {
        fprintf(stderr, "IR generation failed\n");
        arena_destroy(arena);
        free(source);
        return 1;
    }

    opt_run_passes(prog);

    FILE *out = fopen(output_path, "w");
    if (!out) {
        fprintf(stderr, "Could not open output file \"%s\"\n", output_path);
        arena_destroy(arena);
        free(source);
        return 1;
    }

    int err = cgen_emit_x86_64(out, prog);
    fclose(out);

    if (err) {
        fprintf(stderr, "Code generation failed\n");
    }

    arena_destroy(arena);
    free(source);

    return err;
}

