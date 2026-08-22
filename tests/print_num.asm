; macOS x86_64, NASM syntax

global print_int

; section for uninitialized static memory.
section .bss
   buffer: resb 21 ; reserves 21 raw, unnamed bytes at label buffer, zeroed/uninitialized by the OS.
   ; 21 bytes covers a 64-bit number's max digit count (up to 20 digits) plus one newline.

; tells the cpu this is readonly code, i.e. executable instructions
section .text
print_int:
   ; --- assumes number is already in rax ---

   lea rdi, [rel buffer + 20] ; lea ("load effective address") computes an address without dereferencing it — unlike mov reg, [addr] (which would read memory), lea just does the arithmetic and stores the resulting address into rdi.
   ; So rdi now holds the address of the buffer's last byte (buffer+20, since the buffer spans buffer+0 through buffer+20, 21 bytes total).
   ; also lea is more efficient that mov ing for addresses bcz of complex OS anti-hacking stuff
   ; we use rel because of that too. it means relative address so os knows it but hackers don't

   mov byte [rdi], 10 ; Writes the newline character (ASCII 10 = '\n') into that last byte. [] operator is basically the dereferencing op
   ; we're building the output string BACKWARDS because we don't know how many digits there are in the number yet
   ; but we know where it ends
   ; we use byte because whenever we dereference an address we must specify the size of the value inside.
   ; only if the value we're moving is a literal. If it's a register then it's not needed cuz ax = 16, eax = 32, rax = 64

   mov rcx, 10 ; the divisor

.digit_loop: ; start of a do-while loop
; preceeded by a dot means this label belongs to whichever function was just before it
; in our case main
; the while condition is checked later with test and jnz

   dec rdi ; move the ptr to the left
   xor rdx, rdx ; make rdx 0 but in a more cpu-efficient way
   ; we're prepping rdx for division. Since we're doing unsigned division, we don't need cqo just 0
   ; rax will remain as it is cuz it contains the number we want
   div rcx ; first iteration: rax = 1234, rdx = 5. Basically we're extracting each digit in this loop

   ; now rdx contains a single-digit number, which mathematically can only take up 1 byte.
   ; which is the last byte of rdx, and it has a name "dl"
   ; if we add the ascii character '0' to this last byte, we will have the ascii character for our number
   ; because of ascii's helpful contiguousness in its number assignments.
   add dl, '0' ; nasm compiler understands single quotes (How helpful!)

   mov [rdi], dl ; write the ascii char into the current buffer position

   ; THE WHILE CHECK
   test rax, rax ; basically does zero flag = rax & rax which will only be 0 if rax is 0
   jnz .digit_loop ; if zero flag is not zero i.e. rax is not zero, jump back to .digit_loop

; WE ARE OUTSIDE THE LOOP NOW
; STILL INSIDE _main

   lea rsi, [rel buffer + 21] ; 1 + the last memory address of buffer (including newline, so we are completely outside buffer here)
   sub rsi, rdi ; rsi = rsi - rdi = last_add+1 - first_add = length in BYTES which is now stored in rsi
   ; for our example, "123456\n" = 6 bytes, so rsi = 6
   ; no specific reason to use rsi, we can use any register cuz this is gonna get moved out anyways

   mov rdx, rsi ; store length in rdx (arg3, which is required there by the write syscall)
   mov rsi, rdi ; store pointer to buffer (first byte's address) in arg2
   mov rdi, 1 ; store the stdout file descriptor in arg1
   mov rax, 4 | 0x2000000 ; write syscall id for mac
   syscall ; write it to the terminal

   ret ; go back to caller
