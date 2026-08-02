; macOS x86_64, NASM syntax

global _main

section .text
_main:
   mov eax, 40    ; store return value
   ret            ; exit with stored value
