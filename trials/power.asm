; macOS x86_64, NASM syntax

extern exponentiate
extern print_int

global _main
_main:
   mov rax, 5
   mov rbx, 3
   call exponentiate
   call print_int ; should print 125

   mov rax, 1 | 0x2000000
   mov rdi, 0
   syscall
