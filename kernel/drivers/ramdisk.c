#include "ramdisk.h"
#include "../mm/vmm.h"
#include "../kprintf.h"

// The image is already in RAM, so "reading a sector" is just a copy.
// block_read() has already checked lba/count against dev->sectors.
static int ramdisk_read(struct block_device* dev, uint64_t lba, uint32_t count, void* buf) {
    const uint8_t* src = (const uint8_t*)dev->priv + lba * BLOCK_SECTOR_SIZE;
    uint8_t* dst = (uint8_t*)buf;
    uint64_t bytes = (uint64_t)count * BLOCK_SECTOR_SIZE;
    for (uint64_t i = 0; i < bytes; i++) dst[i] = src[i];
    return 0;
}

int ramdisk_init(struct block_device* dev, uint64_t phys, uint64_t size) {
    if (size < BLOCK_SECTOR_SIZE) {
        kprintf("ramdisk: module too small (%d bytes)\n", (int)size);
        return -1;
    }
    dev->name    = "ram0";
    dev->sectors = size / BLOCK_SECTOR_SIZE;
    dev->read    = ramdisk_read;
    dev->priv    = phys_to_virt(phys);      // reach it through the HHDM
    kprintf("ramdisk: ram0 = %d KB at phys %p (%d sectors)\n",
            (int)(size / 1024), (void*)phys, (int)dev->sectors);
    return 0;
}