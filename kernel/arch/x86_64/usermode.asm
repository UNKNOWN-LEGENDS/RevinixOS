bits 64
default rel

global jump_usermode
extern kprintf

; void jump_usercode(uint64_t entry, uint64_t user_stack)
; rdi = user code entry point
; rsi = user stack top
jump_usermode:
	; load user data selector (0x20 | RPL 3 = 0x23) into segment regs
	mov ax, 0x23
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; build a fake iretq frame (pushed in reverse of pop order);
	; iretq pops: RIP, CS, RFLAGS, RSP, SS
	push qword 0x23		; SS = user data | RPL 3
	push rsi 		; RSP = user stack top
	push qword 0x202	; RFLAGS = reserved bit + IF (interrupts enabled)
	push qword 0x1B		; CS = user code | RPL 3
	push rdi		; RIP = user entry point
	iretq			; "return" into Ring 3


	; debug: print the entry and stack we're abotu to iretq into
	; (rdi = entry, rsi = user stack - still intact)
	; call a C helper before iretq
	push rdi
	push rsi
	mov rdi, [rsp+16]	; not strictly needed; simpler to print in c
	pop rsi
	pop rdi

	iretq
