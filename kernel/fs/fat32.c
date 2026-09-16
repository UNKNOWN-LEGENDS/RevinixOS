#include "fat32.h"
#include "../drivers/ata.h"
#include "../kprintf.h"

int fat32_init(struct fat32_fs* fs) {
    uint8_t sector[ATA_SECTOR_SIZE];

    // Boot sector / BPB lives at LBA 0.
    if (ata_read(0, 1, sector) != 0) {
        kprintf("fat32: failed to read boot sector\n");
        return -1;
    }

    // The last two bytes of the boot sector must be the 0x55 0xAA signature.
    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        kprintf("fat32: bad boot signature (%x %x), not a formatted volume\n",
                sector[510], sector[511]);
        return -1;
    }

    const struct fat32_bpb* bpb = (const struct fat32_bpb*)sector;

    // A couple of sanity checks that distinguish FAT32 from FAT12/16:
    // on FAT32, the 16-bit root-entry-count and 16-bit fat-size are zero.
    if (bpb->root_entry_count != 0 || bpb->fat_size_16 != 0) {
        kprintf("fat32: volume looks like FAT12/16, not FAT32\n");
        return -1;
    }
    if (bpb->bytes_per_sector != ATA_SECTOR_SIZE) {
        kprintf("fat32: unexpected sector size %d (driver assumes %d)\n",
                bpb->bytes_per_sector, ATA_SECTOR_SIZE);
        return -1;
    }

    fs->bytes_per_sector    = bpb->bytes_per_sector;
    fs->sectors_per_cluster = bpb->sectors_per_cluster;
    fs->root_cluster        = bpb->root_cluster;
    fs->fat_start_lba       = bpb->reserved_sectors;
    fs->data_start_lba      = bpb->reserved_sectors
                            + (uint32_t)bpb->num_fats * bpb->fat_size_32;

    kprintf("fat32: OK. bytes/sec=%d sec/clus=%d fats=%d fatsize=%d\n",
            (int)bpb->bytes_per_sector, (int)bpb->sectors_per_cluster,
            (int)bpb->num_fats, (int)bpb->fat_size_32);
    kprintf("fat32: fat_start=LBA %d  data_start=LBA %d  root_cluster=%d\n",
            (int)fs->fat_start_lba, (int)fs->data_start_lba, (int)fs->root_cluster);
    return 0;
}