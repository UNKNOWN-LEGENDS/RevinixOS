#include "syscall.h"
#include "../arch/x86_64/idt.h"
#include "../kprintf.h"
#include "../drivers/serial.h"
#include <stdint.h>

extern void syscall_entry(void);
void set_gate_external(int vec, void* handler, uint8_t type_attr);

// task_exit lives in the scheduler; used by SYS_EXIT
void task_exit(void);

// SYS_WRITE: write `count` bytes from `buf` to the console (serial).
// Returns bytes written.
static int64_t sys_write(const char* buf, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        serial_write_char(buf[i]);
    }
    return (int64_t)count;
}

// The C dispatcher. Reads the syscall number and args from the saved frame,
// and writes the return value back into the frame's rax slot.
void syscall_dispatch(struct interrupt_frame* frame) {
    uint64_t num = frame->rax;   // syscall number
    uint64_t a1  = frame->rdi;   // arg1
    uint64_t a2  = frame->rsi;   // arg2
    // uint64_t a3 = frame->rdx; // arg3 (unused for now)

    int64_t ret = -1;

    switch (num) {
        case SYS_WRITE:
            ret = sys_write((const char*)a1, a2);
            break;
        case SYS_EXIT:
            kprintf("\n[SYS_EXIT called by user program, code=%d]\n", (int)a1);
            task_exit();          // does not return
            break;
        default:
            kprintf("[unknown syscall %d]\n", (int)num);
            ret = -1;
            break;
    }

    frame->rax = (uint64_t)ret;   // return value goes back to user in rax
}

void syscall_init(void) {
    // 0xEE = present, DPL 3, 64-bit interrupt gate.
    //   0x8E was DPL 0; the 0x60 difference sets DPL=3 so Ring 3 may call int 0x80.
    set_gate_external(0x80, (void*)syscall_entry, 0xEE);
    kprintf("Syscall interface installed (int 0x80, DPL 3).\n");
}