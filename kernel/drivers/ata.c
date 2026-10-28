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
#define ATA_ALTSTATUS   0x3F6   // same as STATUS, but reading it has no side effects

// Status register bits.
#define ST_ERR  0x01
#define ST_DRQ  0x08   // data request: ready to transfer a sector
#define ST_DF   0x20   // drive fault
#define ST_BSY  0x80   // busy

#define CMD_READ_SECTORS   0x20
#define CMD_WRITE_SECTORS  0x30
#define CMD_CACHE_FLUSH    0xE7
#define CMD_IDENTIFY       0xEC

// Upper bound on status polls before we give up. One inb is roughly 1 us on
// real hardware, so this is on the order of a second. A loop count, not a
// real clock: good enough to turn "hang forever" into "fail with a message".
#define ATA_TIMEOUT 1000000u

static int      ata_present   = 0;   // set by a successful ata_init()
static int      ata_writes_ok = 0;   // writes refused until explicitly allowed
static uint32_t ata_sectors   = 0;   // LBA28 sector count from IDENTIFY

// ~400 ns delay: four reads of the alternate status port.
static void ata_delay400(void) {
    for (int i = 0; i < 4; i++) (void)inb(ATA_ALTSTATUS);
}

// Wait for BSY to clear. Returns the final status byte, or -1 on timeout or
// on a floating bus (0xFF = no controller is decoding these ports).
static int ata_wait_not_busy(void) {
    for (uint32_t i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t s = inb(ATA_STATUS);
        if (s == 0xFF)     return -1;
        if (!(s & ST_BSY)) return s;
    }
    return -1;
}

// Wait until the drive is ready to move a sector (DRQ), with a timeout.
// Returns 0 on ready, -1 on error/fault/timeout.
static int ata_poll(void) {
    int s = ata_wait_not_busy();
    if (s < 0) return -1;
    for (uint32_t i = 0; i < ATA_TIMEOUT; i++) {
        if (s & (ST_ERR | ST_DF)) return -1;
        if (s & ST_DRQ)           return 0;
        s = inb(ATA_STATUS);
    }
    return -1;
}

int ata_init(void) {
    ata_present = 0;

    // 1) Floating bus: if nothing decodes 0x1F0-0x1F7 (e.g. SATA in AHCI
    //    mode), every read returns 0xFF. Polling that for BSY hangs forever.
    if (inb(ATA_STATUS) == 0xFF) {
        kprintf("ata: no legacy IDE controller (floating bus) - disk disabled\n");
        return -1;
    }

    // 2) Select master, clear the address registers, send IDENTIFY.
    outb(ATA_DRIVE, 0xA0);
    ata_delay400();
    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LO,   0);
    outb(ATA_LBA_MID,  0);
    outb(ATA_LBA_HI,   0);
    outb(ATA_COMMAND,  CMD_IDENTIFY);
    ata_delay400();

    if (inb(ATA_STATUS) == 0) {
        kprintf("ata: no drive on primary master - disk disabled\n");
        return -1;
    }
    if (ata_wait_not_busy() < 0) {
        kprintf("ata: IDENTIFY timed out - disk disabled\n");
        return -1;
    }
    // ATAPI (CD/DVD) and SATA-signature devices put non-zero values here.
    if (inb(ATA_LBA_MID) != 0 || inb(ATA_LBA_HI) != 0) {
        kprintf("ata: primary master is not a plain ATA disk - disk disabled\n");
        return -1;
    }
    if (ata_poll() != 0) {
        kprintf("ata: IDENTIFY failed (status=%x) - disk disabled\n", inb(ATA_STATUS));
        return -1;
    }

    // 3) Read the 256-word IDENTIFY block.
    uint16_t id[256];
    insw(ATA_DATA, id, 256);

    if (!(id[49] & (1 << 9))) {
        kprintf("ata: drive does not support LBA - disk disabled\n");
        return -1;
    }
    ata_sectors = (uint32_t)id[60] | ((uint32_t)id[61] << 16);
    if (ata_sectors == 0) {
        kprintf("ata: drive reports 0 LBA28 sectors - disk disabled\n");
        return -1;
    }

    // Model string: words 27-46, two ASCII chars per word, high byte first.
    char model[41];
    for (int i = 0; i < 20; i++) {
        model[2 * i]     = (char)(id[27 + i] >> 8);
        model[2 * i + 1] = (char)(id[27 + i] & 0xFF);
    }
    model[40] = '\0';
    for (int i = 39; i >= 0 && model[i] == ' '; i--) model[i] = '\0';

    kprintf("ata: primary master \"%s\", %d sectors (%d MB), LBA28 PIO\n",
            model, (int)ata_sectors, (int)(ata_sectors / 2048));
    ata_present = 1;
    return 0;
}

void ata_allow_writes(int on) {
    ata_writes_ok = on ? 1 : 0;
}

// Common checks for read/write. Returns 0 if the request may proceed.
static int ata_check(uint32_t lba, uint8_t count, const char* op) {
    if (!ata_present) {
        kprintf("ata_%s: no disk\n", op);
        return -1;
    }
    if (count == 0 || lba >= ata_sectors || count > ata_sectors - lba) {
        kprintf("ata_%s: lba=%d count=%d is outside the disk (%d sectors)\n",
                op, (int)lba, (int)count, (int)ata_sectors);
        return -1;
    }
    return 0;
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
    if (ata_check(lba, count, "read") != 0) return -1;

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
    if (!ata_writes_ok) {
        kprintf("ata_write: REFUSED (writes disabled) lba=%d\n", (int)lba);
        return -1;
    }
    if (ata_check(lba, count, "write") != 0) return -1;

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
    if (ata_wait_not_busy() < 0) {
        kprintf("ata_write: cache flush timed out\n");
        return -1;
    }
    return 0;
}
