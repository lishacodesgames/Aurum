// mac arm64 syntax
// on linux, it would be smth like .global main or global _start
.globl _main // makes _main visible outside, meaning it can be called from other .o files, etc.
_main:
   movl $40, %eax // store return value 
   ret // return with value stored in w0
