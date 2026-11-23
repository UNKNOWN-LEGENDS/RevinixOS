; ptrtest.asm - hands the kernel bad pointers and a bad fd on purpose.
; Every bad call must come back with the right -errno instead of crashing the
; kernel or leaking kernel memory. Exit code = number of checks that FAILED.
%include "sys.inc"
bits 64

; TRY fd, buffer, length, errno, label, label_len
%macro TRY 6
    PRINT %5, %6
    WRITE %1, %2, %3
    cmp rax, -%4
    je %%rejected
    PRINT bad, bad_len
    inc r15
    jmp %%done
%%rejected:
    PRINT ok, ok_len
%%done:
%endmacro

section .text
global _start
_start:
    xor r15, r15                                                ; failure count
    TRY STDOUT, 0xFFFFFFFF80100000, 16,      EFAULT, t1, t1_len ; kernel code address
    TRY STDOUT, 0,                  16,      EFAULT, t2, t2_len ; NULL
    TRY STDOUT, msg,                0x10000, EFAULT, t3, t3_len ; starts valid, runs past our pages
    TRY STDOUT, 0xFFFFFFFFFFFFFFF0, 32,      EFAULT, t4, t4_len ; pointer + length wraps past 2^64
    TRY STDOUT, 0x00007FFFFFFFFFF0, 32,      EFAULT, t5, t5_len ; crosses the top of user space
    TRY 7,      msg,                msg_len, EBADF,  t6, t6_len ; fd 7 doesn't exist

    PRINT msg, msg_len                                          ; a normal write still works
    EXIT r15

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
t6:  db "[ptrtest] bad fd 7 ............ "
t6_len equ $ - t6
ok:  db "rejected", 0x0A
ok_len equ $ - ok
bad: db "NOT REJECTED", 0x0A
bad_len equ $ - bad
msg: db "[ptrtest] valid write still works", 0x0A
msg_len equ $ - msg