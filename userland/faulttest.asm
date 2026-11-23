; faulttest.asm - reads address 0. The kernel should kill this program and
; return to the shell instead of halting the whole machine.
bits 64

%define SYS_WRITE 1
%define SYS_EXIT  2

%macro PRINT 2
    mov rax, SYS_WRITE
    mov rdi, %1
    mov rsi, %2
    int 0x80
%endmacro

section .text
global _start
_start:
    PRINT m1, m1_len
    mov rax, [0]                ; NULL read -> page fault
    PRINT m2, m2_len            ; must never print
    mov rax, SYS_EXIT
    xor rdi, rdi
    int 0x80
.hang:
    jmp .hang

section .rodata
m1: db "[faulttest] reading address 0 ...", 0x0A
m1_len equ $ - m1
m2: db "[faulttest] STILL ALIVE - fault was not caught", 0x0A
m2_len equ $ - m2