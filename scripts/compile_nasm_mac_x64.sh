#!/usr/bin/env bash
set -euo pipefail # stop if a command fails, if a variable is unset, or if a command in a pipe fails

# require ATLEAST 1 argument, but can accept multiple asm files for compilation
if [[ $# -lt 1 ]]; then
   echo "Incorrect usage!" >&2
   echo "Correct usage: $0 <file.asm> [<file2.asm> ...]" >&2
   exit 1
fi

outdir="./out"
mkdir -p "$outdir" # does nothing if already exists

objects=() # array in case multiple asm files

# loop over every argument passed to the script (each should be a .asm file)
for src in "$@"; do
   if [[ $src != *.asm ]]; then
      echo "Skipping non-.asm file: $src" >&2
      continue # skip anything that isn't a .asm file, rather than failing the whole build
   fi

   filename=$(basename -- "$src") # program.asm
   name=${filename%.asm}        # program
   object="/tmp/$name.o"

   # Compile the assembly file into an object file
   nasm -f macho64 -o "$object" "$src"

   objects+=("$object") # save path

done

first_filename=$(basename -- "$1")
executable_name=${first_filename%.asm}
outpath="$outdir/$executable_name"

# Link object file into an executable
clang -arch x86_64 -o "$outpath" "${objects[@]}"
