; wxtest.asm - tries to overwrite one byte of its own code. With W^X the
; code page is read-only and the task is killed on the write.
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
    mov byte [rel target], 0x90     ; write into .text
    PRINT m2, m2_len
    mov rax, SYS_EXIT
    xor rdi, rdi
    int 0x80
target:
    nop
.hang:
    jmp .hang

section .rodata
m1: db "[wxtest] writing into our own code ...", 0x0A
m1_len equ $ - m1
m2: db "[wxtest] write SUCCEEDED - code is NOT write-protected", 0x0A
m2_len equ $ - m2