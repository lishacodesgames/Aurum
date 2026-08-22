; macOS x86_64, NASM syntax

global exponentiate

exponentiate:
   ; --- assumes: rax = base, rbx = exponent ---
   ; returns result in rax

   mov rcx, rax ; save the base, since rax will store the answer
   mov rax, 1

.pow_loop:
;  while rbx != 0 : {
;     result *= base
;     exponent--
;  }

   ; we use while and not do-while because rbx can be 0, in that case result should be 1
   test rbx, rbx
   jz .pow_done ; exit if rbx == 0

   imul rax, rcx
   dec rbx  ; sub rbx, 1
   jmp .pow_loop

.pow_done:
   ret
