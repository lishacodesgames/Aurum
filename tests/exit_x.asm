; macOS x86_64, NASM syntax

; @note r prefix is for 64-bit registers, e.g. rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp
global _main
_main:
   ; currently, rbp (64-bit base pointer) points to whoever called us
   ; we want to save that and make our own stack frame where we can store our variables
   ; a stack frame is just a chunk of memory (btwn rbp and rsp, stack pointer) that we can use to store local variables

   push rbp               ; save the caller's base pointer
   mov rbp, rsp           ; set our base pointer to the current stack pointer
   sub rsp, 16            ; allocate 16 bytes on the stack for 2 local variables

   ; (stack grows downwards, so we subtract from rsp to allocate space)
   ; current stack:
   ;
   ; |--------------------| <- rbp (stays constant in this entire function scope)
   ; |  local variable 1  | <- address = rbp - 8
   ; |  local variable 2  | <- address = rbp - 16
   ; |--------------------| <- rsp (can move up or down based on how much we need). Currently empty

   ; QWORD = 8-byte value, [address] = dereferencing it, 5 = value put in slot
   mov QWORD [rbp - 8], 5 ; put 5 into first variable slot

   ; rdi is the "first argument" register, and exit() takes exactly 1 argument, hence we store the exit code in rdi
   ; the arg registers in order are: rdi, rsi, rdx, r10 (not rcx), r8, r9
   ; if there are more than 6 arguments to a function, you're cooked
   ; but the in-built ones don't.

   mov rdi, [rbp - 8]     ; load exit code from a variable
   mov rax, 1 | 0x2000000 ; exit syscode
   syscall