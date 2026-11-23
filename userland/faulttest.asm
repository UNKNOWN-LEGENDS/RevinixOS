; faulttest.asm - reads address 0. The kernel should kill this program and
; return to the shell instead of halting the whole machine.
%include "sys.inc"
bits 64

section .text
global _start
_start:
    PRINT m1, m1_len
    mov rax, [0]                ; NULL read -> page fault
    PRINT m2, m2_len            ; must never print
    EXIT 0

section .rodata
m1: db "[faulttest] reading address 0 ...", 0x0A
m1_len equ $ - m1
m2: db "[faulttest] STILL ALIVE - fault was not caught", 0x0A
m2_len equ $ - m2