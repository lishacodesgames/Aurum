#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
   echo "Usage: $0 <file.asm>" >&2
   exit 1
fi

input=$1

if [[ $input != *.asm ]]; then
   echo "Error: argument must be an .asm file" >&2
   exit 1
fi

filename=$(basename -- "$input")   # e.g. program.asm
name=${filename%.asm}              # e.g. program

clang \
   -x assembler \
   -arch x86_64 \
   -o "$name" \
   "$filename"
