; nxtest.asm - writes 6 bytes of machine code onto its stack and calls them.
; Without NX they run and return 42. With NX the CPU refuses to fetch
; instructions from the (non-executable) stack page and the task is killed.
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
    sub rsp, 16
    mov dword [rsp], 0x00002AB8     ; bytes B8 2A 00 00 ...
    mov word  [rsp + 4], 0xC300     ; ... 00 C3  =  mov eax, 42 ; ret
    mov rbx, rsp
    call rbx                        ; jump into the stack
    cmp eax, 42
    jne .exit
    PRINT m2, m2_len
.exit:
    mov rax, SYS_EXIT
    xor rdi, rdi
    int 0x80
.hang:
    jmp .hang

section .rodata
m1: db "[nxtest] copying code onto the stack and calling it ...", 0x0A
m1_len equ $ - m1
m2: db "[nxtest] stack code RAN - NX is NOT enforced", 0x0A
m2_len equ $ - m2