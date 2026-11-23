#include "file.h"
#include "../mm/heap.h"
#include "../syscall/syscall.h"

// One console object shared by every process's fd 0, 1 and 2. It's static,
// so closing those fds never frees it.
static struct file console = { .kind = FILE_CONSOLE };

void fd_table_init(struct file** fds) {
    for (int i = 0; i < MAX_FDS; i++) fds[i] = 0;
    fds[0] = fds[1] = fds[2] = &console;
}

static void file_release(struct file* f) {
    if (f && f != &console) kfree(f);
}

void fd_table_close_all(struct file** fds) {
    for (int i = 0; i < MAX_FDS; i++) {
        file_release(fds[i]);
        fds[i] = 0;
    }
}

struct file* fd_get(struct file** fds, int64_t fd) {
    if (fd < 0 || fd >= MAX_FDS) return 0;
    return fds[fd];
}

int fd_install(struct file** fds, struct file* f) {
    for (int i = 0; i < MAX_FDS; i++)
        if (!fds[i]) { fds[i] = f; return i; }
    return -EMFILE;
}

int fd_close(struct file** fds, int64_t fd) {
    struct file* f = fd_get(fds, fd);
    if (!f) return -EBADF;
    file_release(f);
    fds[fd] = 0;
    return 0;
}

struct file* file_open(const char* name, int* err) {
    struct file* f = (struct file*)kmalloc(sizeof(struct file));
    if (!f) { *err = -ENOENT; return 0; }
    if (vfs_open(name, &f->vf) != 0) {
        kfree(f);
        *err = -ENOENT;
        return 0;
    }
    f->kind   = FILE_VFS;
    f->offset = 0;
    return f;
}