; badelf.asm - a normal little program. The Makefile links it TWICE at
; addresses the loader must refuse:
;   LOWELF.ELF  at 0x1000               (inside the never-mapped NULL area)
;   KERNELF.ELF code at 0x400000 (fine) but its read-only data at
;               0xFFFF800000100000      (the kernel half!)
; If either ever runs, it prints the line below.
%include "sys.inc"
bits 64

section .text
global _start
_start:
    WRITE STDOUT, msg, msg_len      ; 64-bit absolute address: reaches anywhere
    EXIT 0

section .rodata
msg: db "[badelf] I SHOULD NOT HAVE BEEN LOADED", 0x0A
msg_len equ $ - msg