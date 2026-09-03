; macOS x86_64, NASM syntax

global _main
_main:
   mov rax, 1 | 0x2000000 ; 1 = exit, 0x2000000 = syscall, can also be written as 0x2000001 but this is more readable
   mov rdi, 40            ; exit code 
   syscall                ; invoke operating system to exit
