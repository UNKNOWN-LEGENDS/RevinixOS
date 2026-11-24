; cattest.asm - exercises open / read / close.
;  1. cat README.TXT in 32-byte reads (several reads, offsets advance)
;  2. read all of HELLO.ELF in 100-byte reads and print the total
;     (it spans many clusters, so this checks reads across cluster borders)
;  3. error cases: each must return the right -errno
; Exit code = number of checks that FAILED.
%include "sys.inc"
bits 64

; CHECK errno, label, label_len   (call result is in rax)
%macro CHECK 3
    push rax
    PRINT %2, %3
    pop rax
    cmp rax, -%1
    je %%ok
    PRINT bad, bad_len
    inc r15
    jmp %%done
%%ok:
    PRINT ok, ok_len
%%done:
%endmacro

section .text
global _start
_start:
    xor r15, r15                    ; failure count

    ; ---- 1. cat README.TXT ----
    OPEN readme, O_RDONLY
    test rax, rax
    js .open_failed
    mov r12, rax                    ; fd
.cat_loop:
    READ r12, buf, 32
    test rax, rax
    jz .cat_done                    ; 0 = end of file
    js .read_failed
    mov r13, rax
    WRITE STDOUT, buf, r13
    jmp .cat_loop
.cat_done:
    CLOSE r12
    PRINT nl, 1                     ; README.TXT has no newline at the end

    ; ---- 2. HELLO.ELF size, counted with 100-byte reads ----
    OPEN hello, O_RDONLY
    test rax, rax
    js .open_failed
    mov r12, rax
    xor r14, r14                    ; total bytes
.count_loop:
    READ r12, buf, 100
    test rax, rax
    jz .count_done
    js .read_failed
    add r14, rax
    jmp .count_loop
.count_done:
    CLOSE r12
    PRINT total_msg, total_len
    mov rax, r14
    call print_dec
    PRINT bytes_msg, bytes_len

    ; ---- 3. error cases ----
    READ r12, buf, 16               ; r12 was just closed
    CHECK EBADF, e1, e1_len
    OPEN missing, O_RDONLY
    CHECK ENOENT, e2, e2_len
    OPEN 0xFFFF800000000000, O_RDONLY       ; path pointer into the kernel
    CHECK EFAULT, e3, e3_len
    OPEN readme, O_RDONLY
    mov r12, rax
    READ r12, 0xFFFFFFFF80100000, 16        ; buffer in the kernel
    CHECK EFAULT, e4, e4_len
    READ r12, readme, 4                     ; buffer in our own read-only data
    CHECK EFAULT, e5, e5_len
    CLOSE r12
    READ STDIN, 0xFFFFFFFF80100000, 16      ; stdin into the kernel: refused before waiting
    CHECK EFAULT, e6, e6_len

    ; open README.TXT until the fd table is full (fds 3..15 = 13 opens)
    mov rbx, 20
.fill:
    OPEN readme, O_RDONLY
    test rax, rax
    js .filled
    dec rbx
    jnz .fill
.filled:
    CHECK EMFILE, e7, e7_len
    ; (they're left open on purpose: exiting must close them all)

    EXIT r15

.open_failed:
    PRINT f_open, f_open_len
    EXIT 100
.read_failed:
    PRINT f_read, f_read_len
    EXIT 101

; print_dec: print rax as an unsigned decimal number
print_dec:
    mov rdi, numbuf + 20            ; fill the buffer from the end
    mov rcx, 10
.digit:
    xor rdx, rdx
    div rcx                         ; rax = rax / 10, rdx = remainder
    add dl, '0'
    dec rdi
    mov [rdi], dl
    test rax, rax
    jnz .digit
    mov rdx, numbuf + 20
    sub rdx, rdi                    ; length
    mov rsi, rdi
    mov rdi, STDOUT
    mov rax, SYS_WRITE
    int 0x80
    ret

section .rodata
nl:      db 0x0A
readme:  db "readme.txt", 0         ; lowercase on purpose: lookup is case-insensitive
hello:   db "/HELLO.ELF", 0         ; leading '/' is accepted too
missing: db "NOPE.TXT", 0
total_msg: db "[cattest] HELLO.ELF read in 100-byte pieces: "
total_len equ $ - total_msg
bytes_msg: db " bytes", 0x0A
bytes_len equ $ - bytes_msg
e1: db "[cattest] read closed fd ...... "
e1_len equ $ - e1
e2: db "[cattest] open missing file ... "
e2_len equ $ - e2
e3: db "[cattest] open kernel pointer . "
e3_len equ $ - e3
e4: db "[cattest] read into kernel .... "
e4_len equ $ - e4
e5: db "[cattest] read into r/o data .. "
e5_len equ $ - e5
e6: db "[cattest] stdin into kernel ... "
e6_len equ $ - e6
e7: db "[cattest] fd table full ....... "
e7_len equ $ - e7
ok:  db "correct error", 0x0A
ok_len equ $ - ok
bad: db "WRONG RESULT", 0x0A
bad_len equ $ - bad
f_open: db "[cattest] open failed", 0x0A
f_open_len equ $ - f_open
f_read: db "[cattest] read failed", 0x0A
f_read_len equ $ - f_read

section .bss
buf:    resb 128                    ; writable: our first rw- data segment
numbuf: resb 20