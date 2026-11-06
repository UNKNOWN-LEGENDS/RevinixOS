; ssetest.asm - checks that our SSE registers survive being preempted.
; Fills xmm0-xmm15 with values only this run uses, burns CPU so the timer
; switches to other tasks many times, then checks every register.
; Run two copies at once:  run SSETEST.ELF SSETEST.ELF
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
    rdtsc                       ; time-stamp counter: differs between the two copies
    shl rdx, 32
    or  rax, rdx
    mov r12, rax                ; our pattern seed (GPRs are saved on every switch)

%assign i 0
%rep 16
    lea rax, [r12 + i]
    movq xmm%[i], rax           ; xmm<i> = seed + i
%assign i i+1
%endrep

    mov r13, 4                  ; check 4 times
.round:
    mov rcx, 0x3000000          ; spin long enough to be preempted many times
.spin:
    dec rcx
    jnz .spin

%assign i 0
%rep 16
    movq rax, xmm%[i]
    lea rbx, [r12 + i]
    cmp rax, rbx
    jne .corrupt
%assign i i+1
%endrep

    PRINT ok, ok_len
    dec r13
    jnz .round

    mov rax, SYS_EXIT
    xor rdi, rdi
    int 0x80

.corrupt:
    PRINT bad, bad_len
    mov rax, SYS_EXIT
    mov rdi, 1
    int 0x80
.hang:
    jmp .hang

section .rodata
ok:  db "[ssetest] xmm0-xmm15 intact", 0x0A
ok_len equ $ - ok
bad: db "[ssetest] XMM REGISTERS CORRUPTED by another task", 0x0A
bad_len equ $ - bad