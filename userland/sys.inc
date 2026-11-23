; sys.inc - RevinixOS syscall numbers and helper macros for asm programs.
; Calling convention: rax = syscall number, rdi / rsi / rdx = arguments,
; result in rax (negative = -errno). Every other register is preserved.

%define SYS_WRITE 1
%define SYS_EXIT  2
%define SYS_OPEN  3
%define SYS_READ  4
%define SYS_CLOSE 5

%define O_RDONLY  0

%define STDIN  0
%define STDOUT 1
%define STDERR 2

%define EBADF  9        ; bad file descriptor
%define EFAULT 14       ; bad address
%define EINVAL 22       ; invalid argument
%define ENOENT 2        ; no such file
%define EMFILE 24       ; too many open files
%define ENAMETOOLONG 36 ; path too long
%define ENOSYS 38       ; not implemented

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

; OPEN path, flags          -> rax = new fd, or -errno
%macro OPEN 2
    mov rax, SYS_OPEN
    mov rdi, %1
    mov rsi, %2
    int 0x80
%endmacro

; READ fd, buf, len         -> rax = bytes read (0 = end of file), or -errno
%macro READ 3
    mov rax, SYS_READ
    mov rdi, %1
    mov rsi, %2
    mov rdx, %3
    int 0x80
%endmacro

; CLOSE fd                  -> rax = 0, or -errno
%macro CLOSE 1
    mov rax, SYS_CLOSE
    mov rdi, %1
    int 0x80
%endmacro

; EXIT code                 -> never returns
%macro EXIT 1
    mov rax, SYS_EXIT
    mov rdi, %1
    int 0x80
%%hang:
    jmp %%hang
%endmacro