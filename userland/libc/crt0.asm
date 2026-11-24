; crt0.asm - "C runtime zero": the first code a C program runs.
; The kernel jumps to _start with rsp = top of the stack. We call main()
; and hand its return value to exit(), so `return 7;` becomes exit code 7.
bits 64
section .text
global _start
extern main
extern exit

_start:
    xor rbp, rbp            ; end of the frame-pointer chain (for debuggers)
    and rsp, -16            ; the ABI wants rsp 16-byte aligned before a call
    xor edi, edi            ; argc = 0  (no arguments yet)
    xor esi, esi            ; argv = NULL
    call main
    mov edi, eax            ; main's return value ...
    call exit               ; ... becomes the exit code (exit never returns)
.hang:
    jmp .hang