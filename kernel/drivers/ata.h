#ifndef ATA_H
#define ATA_H
#include <stdint.h>

#define ATA_SECTOR_SIZE 512

// Primary bus, LBA28, polled PIO. Primary master only.
//
// Safety rules (Stage 2a-0):
//   - ata_init() must succeed (controller + IDENTIFY) before any read/write.
//   - Every wait has a timeout, so a missing controller can't hang the kernel.
//   - Writes are REFUSED unless ata_allow_writes(1) was called first.
//   - Reads/writes outside the disk's reported size are refused.

// Probe the primary master. Returns 0 if a plain ATA disk answered IDENTIFY.
int  ata_init(void);

// Turn writes on/off (default: off). Only test code should turn this on.
void ata_allow_writes(int on);

// Returns 0 on success, non-zero on error. `count` sectors, 1..255.
int ata_read (uint32_t lba, uint8_t count, void* buf);
int ata_write(uint32_t lba, uint8_t count, const void* buf);

#endif
