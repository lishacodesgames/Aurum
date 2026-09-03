; macOS x86_64, NASM syntax

global _main
_main:
   mov rdi, 5
   mov rbx, 5

   imul rdi, rbx ; reg1 = reg1 * reg2, imul for signed numbers
   mov rax, 1 | 0x2000000
   syscall
