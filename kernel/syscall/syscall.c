#include "syscall.h"
#include "../arch/x86_64/idt.h"
#include "../kprintf.h"
#include "../drivers/serial.h"
#include <stdint.h>
#include "../mm/uaccess.h"
#include "../sched/sched.h"
#include "../fs/file.h"
#include "../mm/heap.h"
#include "../drivers/keyboard.h"

extern void syscall_entry(void);
void set_gate_external(int vec, void* handler, uint8_t type_attr);

// SYS_WRITE(fd, buf, count): write `count` bytes from the user buffer to fd.
// Only console fds can be written (files are read-only until FAT32 write).
// Returns bytes written, -EBADF for a closed or read-only fd, or -EFAULT if
// any byte of the buffer isn't valid user memory (nothing printed then).
#define WRITE_MAX   (1024 * 1024)      // one call may print at most 1 MB
#define WRITE_CHUNK 256

static int64_t sys_write(uint64_t fd, uint64_t ubuf, uint64_t count) {
    struct file* f = fd_get(sched_current()->fds, (int64_t)fd);
    if (!f || f->kind != FILE_CONSOLE) return -EBADF;
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

// SYS_OPEN(path, flags): open a file in the root directory, read-only.
// Returns the new fd, or -ENOENT / -EFAULT / -ENAMETOOLONG / -EMFILE / -EINVAL.
#define PATH_MAX 64

static int64_t sys_open(uint64_t upath, uint64_t flags) {
    if (flags != O_RDONLY) return -EINVAL;

    char path[PATH_MAX];
    int len = strncpy_from_user(path, upath, PATH_MAX);
    if (len < 0) return len;

    // Accept "/NAME.EXT" and lowercase: FAT short names are uppercase and
    // looked up case-insensitively.
    char* name = path;
    if (name[0] == '/') name++;
    for (char* c = name; *c; c++)
        if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 'a' + 'A');

    int err = 0;
    struct file* f = file_open(name, &err);
    if (!f) return err;

    int fd = fd_install(sched_current()->fds, f);
    if (fd < 0) kfree(f);                  // table full: don't leak the file
    return fd;
}

// ---- keyboard input (fd 0) ----
// "Line mode", like a Unix terminal: read() waits until the user presses
// Enter, then hands out the line plus '\n'. If the program asks for fewer
// bytes than the line holds, the rest is kept for its next read().
#define CONSOLE_LINE_MAX 130            // 128 chars + '\n' + NUL

static char     con_line[CONSOLE_LINE_MAX];
static uint32_t con_len = 0;            // bytes in con_line
static uint32_t con_pos = 0;            // next byte to hand out

static int64_t console_read(uint64_t ubuf, uint64_t count) {
    if (count == 0) return 0;
    if (!user_range_ok(ubuf, count, 1)) return -EFAULT;   // check BEFORE waiting

    if (con_pos == con_len) {                // nothing left over: wait for a new line
        char line[CONSOLE_LINE_MAX - 1];
        // We're inside int 0x80, so interrupts are off. Turn them on while we
        // wait, or the keyboard IRQ could never deliver the line.
        __asm__ volatile ("sti");
        while (!keyboard_poll_line(line, sizeof(line))) {
            yield();                         // let other tasks run
            __asm__ volatile ("sti; hlt");   // then sleep until the next interrupt
        }
        uint32_t n = 0;
        while (line[n] && n < sizeof(line) - 1) { con_line[n] = line[n]; n++; }
        con_line[n++] = '\n';
        con_len = n;
        con_pos = 0;
    }

    uint64_t take = con_len - con_pos;
    if (take > count) take = count;
    if (copy_to_user(ubuf, con_line + con_pos, take) != 0) return -EFAULT;
    con_pos += (uint32_t)take;
    return (int64_t)take;
}

// SYS_READ(fd, buf, count): read up to `count` bytes from fd into the user
// buffer, advancing the file position. Returns bytes read (0 = end of file),
// -EBADF or -EFAULT (buffer isn't writable user memory). fd 0 waits for a
// line from the keyboard.

#define READ_CHUNK 512

static int64_t sys_read(uint64_t fd, uint64_t ubuf, uint64_t count) {
    struct file* f = fd_get(sched_current()->fds, (int64_t)fd);
    if (!f) return -EBADF;
    if (f->kind == FILE_CONSOLE) return console_read(ubuf, count);
    if (count > WRITE_MAX) count = WRITE_MAX;
    if (!user_range_ok(ubuf, count, 1)) return -EFAULT;     // must be WRITABLE

    uint8_t kbuf[READ_CHUNK];
    uint64_t done = 0;
    while (done < count) {
        uint32_t want = (count - done > READ_CHUNK) ? READ_CHUNK : (uint32_t)(count - done);
        int got = vfs_read_at(&f->vf, f->offset, kbuf, want);
        if (got < 0) return done ? (int64_t)done : -EINVAL;
        if (got == 0) break;                                  // end of file
        if (copy_to_user(ubuf + done, kbuf, (uint64_t)got) != 0)
            return done ? (int64_t)done : -EFAULT;
        f->offset += (uint32_t)got;
        done += (uint64_t)got;
    }
    return (int64_t)done;
}

// SYS_CLOSE(fd): returns 0 or -EBADF.
static int64_t sys_close(uint64_t fd) {
    return fd_close(sched_current()->fds, (int64_t)fd);
}

// The C dispatcher. Reads the syscall number and args from the saved frame,
// and writes the return value back into the frame's rax slot.
void syscall_dispatch(struct interrupt_frame* frame) {
    uint64_t num = frame->rax;   // syscall number
    uint64_t a1  = frame->rdi;   // arg1
    uint64_t a2  = frame->rsi;   // arg2
    uint64_t a3 = frame->rdx; // arg3 (unused for now)

    int64_t ret = -1;

    switch (num) {
        case SYS_WRITE:
            ret = sys_write(a1, a2, a3);
            break;
        case SYS_EXIT:
            task_exit_with((int)a1);          // does not return
            break;
        case SYS_OPEN:
            ret = sys_open(a1, a2);
            break;
        case SYS_READ:
            ret = sys_read(a1, a2, a3);
            break;
        case SYS_CLOSE:
            ret = sys_close(a1);
            break;
        default:
            ret = -ENOSYS;
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