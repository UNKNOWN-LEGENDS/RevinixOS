; echotest.asm - keyboard input through read(0, ...).
;  1. asks for your name and greets you
;  2. reads your next line 4 bytes at a time, printing each piece in [ ],
;     to show the kernel keeps the rest of a line for the next read()
%include "sys.inc"
bits 64

section .text
global _start
_start:
    ; ---- 1. name ----
    PRINT ask, ask_len
    READ STDIN, buf, 64             ; waits for Enter; rax = bytes incl. '\n'
    test rax, rax
    jle .fail
    mov r12, rax
    lea rbx, [rel buf]              ; (buf sits above 4 GB: no 32-bit absolute address)
    cmp byte [rbx + r12 - 1], 0x0A  ; drop the trailing newline
    jne .no_nl
    dec r12
.no_nl:
    PRINT hello, hello_len
    WRITE STDOUT, buf, r12
    PRINT nice, nice_len

    ; ---- 2. small reads ----
    PRINT ask2, ask2_len
.piece:
    READ STDIN, buf, 4
    test rax, rax
    jle .fail
    mov r12, rax
    PRINT lb, 1
    WRITE STDOUT, buf, r12
    PRINT rb, 1
    lea rbx, [rel buf]
    cmp byte [rbx + r12 - 1], 0x0A  ; this piece ended the line?
    jne .piece
    PRINT nl, 1
    EXIT 0

.fail:
    PRINT failmsg, failmsg_len
    EXIT 1

section .rodata
ask:   db "[echotest] What's your name? "
ask_len equ $ - ask
hello: db "[echotest] Hello, "
hello_len equ $ - hello
nice:  db "! Nice to meet you.", 0x0A
nice_len equ $ - nice
ask2:  db "[echotest] Type a sentence; I'll read it 4 bytes at a time:", 0x0A
ask2_len equ $ - ask2
lb:    db "["
rb:    db "]"
nl:    db 0x0A
failmsg: db "[echotest] read failed", 0x0A
failmsg_len equ $ - failmsg

section .bss
buf:   resb 64