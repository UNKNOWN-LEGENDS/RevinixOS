bits 64
section .text
global _start
_start:
    xor r12, r12                ; iteration counter - callee-saved, MUST survive preemption
.loop:
    mov rax, 1                  ; SYS_WRITE
    lea rdi, [rel msg]
    mov rsi, msg_len
    int 0x80


    ; burn time so the 100 Hz timer preepts us mid-loop, in Ring 3.
    ; rcx is caller-saved - it must also survive preemption or hte delay breaks.
    mov rcx, 0x2000000
.delay:
    dec rcx
    jnz .delay

    inc r12
    cmp r12, 8          ; exactly 8 iterations, then exit
    jl .loop

    mov rax, 2          ; SYS_EXIT
    mov rdi, 0
    int 0x80
.hang:
    jmp .hang

section .rodata
msg:    db "[user] ring3 tick", 0x0A
msg_len equ $ - msg