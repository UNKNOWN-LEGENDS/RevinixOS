#ifndef FILE_H
#define FILE_H
#include <stdint.h>
#include "vfs.h"

#define MAX_FDS 16              // per process: fd 0..15

enum file_kind {
    FILE_CONSOLE,               // fd 0/1/2: keyboard in, screen out
    FILE_VFS,                   // a file opened from the filesystem
};

// A kernel "open file": what an fd points at. It remembers the read position.
struct file {
    enum file_kind  kind;
    struct vfs_file vf;         // FILE_VFS only
    uint32_t        offset;     // next byte read() returns
};

// Give a new process its standard fds: 0, 1, 2 -> the console. Others empty.
void fd_table_init(struct file** fds);
// Close every open fd (process exit). Leaves the table empty.
void fd_table_close_all(struct file** fds);

// The file behind fd, or NULL if fd is out of range / not open.
struct file* fd_get(struct file** fds, int64_t fd);
// Put f in the lowest free slot. Returns the fd, or -EMFILE if the table is full.
int  fd_install(struct file** fds, struct file* f);
// Close fd. Returns 0, or -EBADF.
int  fd_close(struct file** fds, int64_t fd);

// Open a filesystem file by name. Returns a new struct file, or NULL
// (*err = -ENOENT if not found).
struct file* file_open(const char* name, int* err);

#endif