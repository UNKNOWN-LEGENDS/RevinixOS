; wxtest.asm - tries to overwrite one byte of its own code. With W^X the
; code page is read-only and the task is killed on the write.
%include "sys.inc"
bits 64

section .text
global _start
_start:
    PRINT m1, m1_len
    mov byte [rel target], 0x90     ; write into .text
    PRINT m2, m2_len
    EXIT 0
target:
    nop

section .rodata
m1: db "[wxtest] writing into our own code ...", 0x0A
m1_len equ $ - m1
m2: db "[wxtest] write SUCCEEDED - code is NOT write-protected", 0x0A
m2_len equ $ - m2