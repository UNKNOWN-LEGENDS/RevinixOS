#ifndef VFS_H
#define VFS_H
#include <stdint.h>
#include "fat32.h"

struct vfs_file;   // forward decl

// Per-filesystem operations. Today only FAT32 fills this in; a second
// filesystem would provide its own table and callers wouldn't change.
struct vfs_ops {
    int (*read)(struct vfs_file* f, uint8_t* buf, uint32_t buf_size);
};

// An open-file handle (kernel-internal; not a userspace fd yet).
struct vfs_file {
    const struct vfs_ops* ops;   // dispatch table for this file's filesystem
    uint32_t size;               // file size in bytes
    int valid;                   // 0 until vfs_open succeeds
    // backend-specific location; for FAT32, the start cluster:
    struct fat32_file fat;
};

// Mount the (single) filesystem the VFS serves. Call once at boot.
void vfs_mount(const struct fat32_fs* fs);

// Open a file by bare name in the mounted filesystem's root.
// Returns 0 and fills *out on success; -1 if not found / not mounted.
int vfs_open(const char* name, struct vfs_file* out);

// Read up to buf_size bytes of the open file into buf.
// Returns bytes read, or -1 on error. Dispatches through out->ops.
int vfs_read(struct vfs_file* f, uint8_t* buf, uint32_t buf_size);

#endif