; ptrtest.asm - hands the kernel bad pointers on purpose.
; Every bad call must come back with -14 (-EFAULT) instead of crashing the
; kernel or leaking kernel memory. The last call is a normal write.
bits 64

%define SYS_WRITE 1
%define SYS_EXIT  2
%define EFAULT    14

; PRINT label, length: write one of our own strings
%macro PRINT 2
    mov rax, SYS_WRITE
    mov rdi, %1
    mov rsi, %2
    int 0x80
%endmacro

; TRY buffer, length, label, label_len: make a write call, report the result
%macro TRY 4
    PRINT %3, %4
    mov rax, SYS_WRITE
    mov rdi, %1
    mov rsi, %2
    int 0x80
    cmp rax, -EFAULT
    je %%rejected
    PRINT bad, bad_len
    jmp %%done
%%rejected:
    PRINT ok, ok_len
%%done:
%endmacro

section .text
global _start
_start:
    TRY 0xFFFFFFFF80100000, 16,      t1, t1_len   ; kernel code address
    TRY 0,                  16,      t2, t2_len   ; NULL
    TRY msg,                0x10000, t3, t3_len   ; starts valid, runs past our pages
    TRY 0xFFFFFFFFFFFFFFF0, 32,      t4, t4_len   ; pointer + length wraps past 2^64
    TRY 0x00007FFFFFFFFFF0, 32,      t5, t5_len   ; crosses the top of user space

    PRINT msg, msg_len                            ; a normal write still works

    mov rax, SYS_EXIT
    xor rdi, rdi
    int 0x80
.hang:
    jmp .hang

section .rodata
t1:  db "[ptrtest] kernel address ...... "
t1_len equ $ - t1
t2:  db "[ptrtest] NULL pointer ........ "
t2_len equ $ - t2
t3:  db "[ptrtest] runs off the end .... "
t3_len equ $ - t3
t4:  db "[ptrtest] wraps around 2^64 ... "
t4_len equ $ - t4
t5:  db "[ptrtest] crosses user limit .. "
t5_len equ $ - t5
ok:  db "rejected (-EFAULT)", 0x0A
ok_len equ $ - ok
bad: db "NOT REJECTED", 0x0A
bad_len equ $ - bad
msg: db "[ptrtest] valid write still works", 0x0A
msg_len equ $ - msg