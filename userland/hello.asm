%include "sys.inc"
bits 64
section .text
global _start
_start:
    xor r12, r12                ; iteration counter - callee-saved, MUST survive preemption
.loop:
    PRINT msg, msg_len

    ; burn time so the 100 Hz timer preempts us mid-loop, in Ring 3.
    ; rcx is caller-saved - it must also survive preemption or the delay breaks.
    mov rcx, 0x2000000
.delay:
    dec rcx
    jnz .delay

    inc r12
    cmp r12, 8                  ; exactly 8 iterations, then exit
    jl .loop

    EXIT 0

section .rodata
msg:    db "[user] ring3 tick", 0x0A
msg_len equ $ - msg