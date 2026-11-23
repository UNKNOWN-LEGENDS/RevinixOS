; sys.inc - RevinixOS syscall numbers and helper macros for asm programs.
; Calling convention: rax = syscall number, rdi / rsi / rdx = arguments,
; result in rax (negative = -errno). Every other register is preserved.

%define SYS_WRITE 1
%define SYS_EXIT  2

%define STDIN  0
%define STDOUT 1
%define STDERR 2

%define EBADF  9        ; bad file descriptor
%define EFAULT 14       ; bad address
%define EINVAL 22       ; invalid argument

; WRITE fd, buf, len        -> rax = bytes written, or -errno
%macro WRITE 3
    mov rax, SYS_WRITE
    mov rdi, %1
    mov rsi, %2
    mov rdx, %3
    int 0x80
%endmacro

; PRINT buf, len            = WRITE STDOUT, buf, len
%macro PRINT 2
    WRITE STDOUT, %1, %2
%endmacro

; EXIT code                 -> never returns
%macro EXIT 1
    mov rax, SYS_EXIT
    mov rdi, %1
    int 0x80
%%hang:
    jmp %%hang
%endmacro