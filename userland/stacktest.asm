; stacktest.asm - uses more and more stack, 4 KB at a time, until it runs
; off the bottom into the guard page. The kernel must kill it with a page
; fault ("stack overflow"), not let it scribble over other memory.
%include "sys.inc"
bits 64

section .text
global _start
_start:
    PRINT m1, m1_len
.grow:
    sub rsp, 4096
    mov qword [rsp], 1              ; touch the new page
    jmp .grow                       ; never ends normally

section .rodata
m1: db "[stacktest] growing the stack 4 KB at a time ...", 0x0A
m1_len equ $ - m1