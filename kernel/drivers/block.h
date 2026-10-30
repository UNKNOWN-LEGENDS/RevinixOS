#ifndef BLOCK_H
#define BLOCK_H
#include <stdint.h>

#define BLOCK_SECTOR_SIZE 512

// A block device: anything that can read numbered 512-byte sectors.
// Filesystems talk to this instead of a specific driver, so FAT32 works the
// same on an ATA disk, a RAM disk, or (later) AHCI / USB storage.
struct block_device {
    const char* name;      // "ata0", "ram0", ...
    uint64_t    sectors;   // device size in 512-byte sectors
    int (*read)(struct block_device* dev, uint64_t lba, uint32_t count, void* buf);
    void*       priv;      // driver-private data
};

// Bounds-checked read. Returns 0 on success, -1 on error / out of range.
static inline int block_read(struct block_device* dev, uint64_t lba,
                             uint32_t count, void* buf) {
    if (!dev || !dev->read) return -1;
    if (count == 0 || lba >= dev->sectors || count > dev->sectors - lba) return -1;
    return dev->read(dev, lba, count, buf);
}

#endif