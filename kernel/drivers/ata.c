#include "ata.h"
#include "../arch/x86_64/io.h"
#include "../kprintf.h"

// Primary ATA bus I/O ports.
#define ATA_DATA        0x1F0   // 16-bit data (read/write sector words here)
#define ATA_ERROR       0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE       0x1F6   // drive/head + LBA[27:24]
#define ATA_STATUS      0x1F7   // read: status  / write: command
#define ATA_COMMAND     0x1F7

// Status register bits.
#define ST_ERR  0x01
#define ST_DRQ  0x08   // data request: ready to transfer a sector
#define ST_DF   0x20   // drive fault
#define ST_BSY  0x80   // busy

#define CMD_READ_SECTORS   0x20
#define CMD_WRITE_SECTORS  0x30
#define CMD_CACHE_FLUSH    0xE7

// Poll until BSY clears; then require DRQ. Returns 0 on ready, -1 on error/fault.
static int ata_poll(void) {
    uint8_t s;
    do { s = inb(ATA_STATUS); } while (s & ST_BSY);
    if (s & (ST_ERR | ST_DF)) return -1;
    if (!(s & ST_DRQ))        return -1;
    return 0;
}

// Wait for BSY to clear (used after a non-data command like CACHE FLUSH).
static void ata_wait_busy(void) {
    while (inb(ATA_STATUS) & ST_BSY) { }
}

// Set up drive select + LBA + sector count for an LBA28 transfer.
static void ata_setup(uint32_t lba, uint8_t count) {
    // 0xE0 = LBA mode, master; top 4 bits of LBA go in the low nibble.
    outb(ATA_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA_LO,   (uint8_t)(lba));
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI,   (uint8_t)(lba >> 16));
}

int ata_read(uint32_t lba, uint8_t count, void* buf) {
    uint16_t* out = (uint16_t*)buf;
    ata_setup(lba, count);
    outb(ATA_COMMAND, CMD_READ_SECTORS);

    for (uint8_t s = 0; s < count; s++) {
        if (ata_poll() != 0) {
            kprintf("ata_read: error at lba=%d sector %d (status=%x)\n",
                    (int)lba, (int)s, inb(ATA_STATUS));
            return -1;
        }
        insw(ATA_DATA, out, ATA_SECTOR_SIZE / 2);   // 256 words = 512 bytes
        out += ATA_SECTOR_SIZE / 2;
    }
    return 0;
}

int ata_write(uint32_t lba, uint8_t count, const void* buf) {
    const uint16_t* in = (const uint16_t*)buf;
    ata_setup(lba, count);
    outb(ATA_COMMAND, CMD_WRITE_SECTORS);

    for (uint8_t s = 0; s < count; s++) {
        if (ata_poll() != 0) {
            kprintf("ata_write: error at lba=%d sector %d (status=%x)\n",
                    (int)lba, (int)s, inb(ATA_STATUS));
            return -1;
        }
        outsw(ATA_DATA, in, ATA_SECTOR_SIZE / 2);
        in += ATA_SECTOR_SIZE / 2;
    }

    // Flush the drive's write cache so the data is committed.
    outb(ATA_COMMAND, CMD_CACHE_FLUSH);
    ata_wait_busy();
    return 0;
}