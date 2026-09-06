#!/usr/bin/env bash
set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input.c> <output_binary>"
    exit 1
fi

INPUT=$1
OUTPUT_BIN=$2
ASM_FILE="${OUTPUT_BIN}.asm"
OBJ_FILE="${OUTPUT_BIN}.o"

./ccompiler "$INPUT" "$ASM_FILE"

nasm -f elf64 "$ASM_FILE" -o "$OBJ_FILE"

gcc -no-pie "$OBJ_FILE" -o "$OUTPUT_BIN"

echo "Successfully built $OUTPUT_BIN"
