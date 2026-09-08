; macOS x86_64, NASM syntax

global exponentiate

exponentiate:
   ; --- arg1: rdi = base, arg2: rsi = exponent ---
   ; --- returns result in rax ---

   mov rax, 1
   test rsi, rsi
   jz .pow_done ; if exponent is 0, result is already 1

.pow_loop:
;  while rsi != 0 : {
;     result *= base
;     exponent--
;  }
; where result = rax, base = rdi, exponent = rsi

   ; we use while and not do-while because exponent can be 0, in that case result should be 1

   imul rax, rdi  ; rax *= rdi
   dec rsi ; if rsi = 0, ZF will be set.
   jnz .pow_loop  ; if exponent != 0, continue loop
   ; jnz checks the last mathematical operation, not any specific register, so we are correctly checking rsi

.pow_done:
   ret ; return with result in rax
