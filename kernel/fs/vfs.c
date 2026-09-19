#include "vfs.h"
#include "../kprintf.h"

// The single mounted filesystem. A registration table would replace this
// when a second filesystem type is added (tracked debt).
static struct fat32_fs mounted_fs;
static int             mounted = 0;

// --- FAT32 backend: the concrete implementation behind the ops table ---
static int fat32_backend_read(struct vfs_file* f, uint8_t* buf, uint32_t buf_size) {
    return fat32_read_file(&mounted_fs, &f->fat, buf, buf_size);
}
static const struct vfs_ops fat32_ops = {
    .read = fat32_backend_read,
};

void vfs_mount(const struct fat32_fs* fs) {
    mounted_fs = *fs;     // copy the geometry in
    mounted = 1;
}

int vfs_open(const char* name, struct vfs_file* out) {
    out->valid = 0;
    if (!mounted) { kprintf("vfs: no filesystem mounted\n"); return -1; }

    if (fat32_find(&mounted_fs, name, &out->fat) != 0)
        return -1;                    // not found

    out->ops   = &fat32_ops;          // wire this handle to the FAT32 backend
    out->size  = out->fat.size;
    out->valid = 1;
    return 0;
}

int vfs_read(struct vfs_file* f, uint8_t* buf, uint32_t buf_size) {
    if (!f->valid || !f->ops || !f->ops->read) return -1;
    return f->ops->read(f, buf, buf_size);   // dispatch — no mention of FAT32 here
}