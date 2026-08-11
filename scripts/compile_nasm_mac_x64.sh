#!/usr/bin/env bash
set -euo pipefail # stop if a command fails, if a variable is unset, or if a command in a pipe fails

if [[ $# -ne 1 || $1 != *.asm ]]; then
   echo "Incorrect usage!" >&2
   echo "Correct usage: $0 <file.asm> [<output directory> (optional)]" >&2
   exit 1
fi

filename=$(basename -- "$1") # program.asm
name=${filename%.asm}        # program
object="/tmp/$name.o"

outdir="./out"
outpath="$outdir/$name"

# Compile the assembly file into an object file
nasm -f macho64 -o "$object" "$1"

# Link object file into an executable
clang -arch x86_64 -o "$outpath" "$object"
