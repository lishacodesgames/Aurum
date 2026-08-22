; macOS x86_64, NASM syntax

extern print_int

global _main
_main:
   ; the division functin in assembly is 'idiv'
   ; but idiv requires a 128bit register as divident, specifically rdx:rax.
   ; basically, you must store the dividend in rax, then extend rax's value into rdx (not by blindly zero-extending, but by using cqo)
   ; cqo CORRECTLY extends rax into rdx based on its signedness

   ; we CANNOT use idiv without modifying both rdx and rax, since idiv only takes the divisor register
   ; QUOTIENT: goes to rax, REMAINDER: goes to rdx

   ; cqo's job: prepare a valid 128-bit input for idiv (this step has nothing to do with quotient/remainder yet — it's purely "make the dividend correctly represent my value at the width idiv requires")
   ; idiv's job: perform the division, and because dividing an integer naturally yields two numbers (how many whole times it divides, and what's left over), it needs two output slots — and reuses rax/rdx for that, same registers, different meaning now

   ; a / b = q, r
   mov rbx, 5 ; the DIVISOR (b)
   mov rax, 16 ; the DIVIDEND (a)
   cqo ; sign-extend rax into rdx. SAME NUMBER, just wider in memory
   idiv rbx ; perform division

   ; now, rax = 3, rdx = 1
   push rdx ; save rdx cuz it's gonna be overritten
   call print_int ; print rax (3)
   pop rax ; print remainder (1)
   call print_int

   mov rdi, rdx
   mov rax, 1 | 0x2000000
   syscall
