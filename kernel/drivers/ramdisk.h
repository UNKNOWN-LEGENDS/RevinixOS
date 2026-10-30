#ifndef RAMDISK_H
#define RAMDISK_H
#include <stdint.h>
#include "block.h"

// Turn a region of physical memory (e.g. a GRUB module) into a read-only
// block device. Returns 0 on success.
int ramdisk_init(struct block_device* dev, uint64_t phys, uint64_t size);

#endif