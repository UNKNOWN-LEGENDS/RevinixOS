bits 64
default rel

extern syscall_dispatch
global syscall_entry

syscall_entry:
	; save registers (same layout as interrupt_frame, minus err/int which we fake)
	push qword 0		; fake err_code (keep from layout uniform)
	push qword 0x80		; "int_no" = 0x80 (informational)

	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov rdi, rsp		; pass pointer to the saved frame
	call syscall_dispatch	; syscall_dispatch writes the return value into the saved rax slot

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax			; <-- picks up the return value the dispatcher stored

	add rsp, 16		; drop fake int_no + err_code
	iretq

