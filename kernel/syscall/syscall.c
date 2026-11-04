#include "syscall.h"
#include "../arch/x86_64/idt.h"
#include "../kprintf.h"
#include "../drivers/serial.h"
#include <stdint.h>
#include "../mm/uaccess.h"

extern void syscall_entry(void);
void set_gate_external(int vec, void* handler, uint8_t type_attr);

// task_exit lives in the scheduler; used by SYS_EXIT
void task_exit(void);

// SYS_WRITE: write `count` bytes from the user buffer `ubuf` to the console.
// Returns bytes written, or -EFAULT if any byte of the buffer isn't valid
// user memory (nothing is printed in that case).
#define WRITE_MAX   (1024 * 1024)      // one call may print at most 1 MB
#define WRITE_CHUNK 256

static int64_t sys_write(uint64_t ubuf, uint64_t count) {
    if (count > WRITE_MAX) return -EINVAL;
    if (!user_range_ok(ubuf, count, 0)) return -EFAULT;   // check it all before printing any

    char kbuf[WRITE_CHUNK];
    uint64_t done = 0;
    while (done < count) {
        uint64_t n = count - done;
        if (n > WRITE_CHUNK) n = WRITE_CHUNK;
        if (copy_from_user(kbuf, ubuf + done, n) != 0)
            return done ? (int64_t)done : -EFAULT;
        for (uint64_t i = 0; i < n; i++) kprintf("%c", kbuf[i]);
        done += n;
    }
    return (int64_t)done;
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
            ret = sys_write(a1, a2);
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