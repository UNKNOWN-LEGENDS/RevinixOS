bits 64
section .text
global _start
_start:
    mov rax, 1              ; SYS_WRITE
    lea rdi, [rel msg]      ; buf   (arg1, your dispatcher reads rdi)
    mov rsi, msg_len        ; count (arg2, your dispatcher reads rsi)
    int 0x80

    mov rax, 2              ; SYS_EXIT
    mov rdi, 0
    int 0x80
.hang:
    jmp .hang
section .rodata
msg:    db "Hello from a loaded ELF binary!", 0x0A
msg_len equ $ - msg
