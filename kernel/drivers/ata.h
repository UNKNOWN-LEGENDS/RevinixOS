#ifndef ATA_H
#define ATA_H
#include <stdint.h>

#define ATA_SECTOR_SIZE 512

//Primary bus, LBA28, polled PIO. Primary master only.
// Returns 0 on success, non-zero on error. `count` sectors, 1..255 (0 => 256 on real HW; keep small here).
int ata_read (uint32_t lba, uint8_t count, void* buf);
int ata_write(uint32_t lba, uint8_t count, const void* buf);

#endif