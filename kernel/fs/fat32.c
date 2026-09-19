#include "fat32.h"
#include "../drivers/ata.h"
#include "../kprintf.h"

static void fat32_format_name(const uint8_t name[11], char* out) {
    int o = 0;
    for (int i = 0; i < 8; i++) {           // base name, stop at padding
        if (name[i] == ' ') break;
        out[o++] = name[i];
    }
    if (name[8] != ' ') {                    // extension, only if present
        out[o++] = '.';
        for (int i = 8; i < 11; i++) {
            if (name[i] == ' ') break;
            out[o++] = name[i];
        }
    }
    out[o] = '\0';
}

void fat32_list_root(const struct fat32_fs* fs) {
    kprintf("fat32: root directory listing:\n");

    uint32_t cluster = fs->root_cluster;
    int guard = 0;

    while (cluster >= 2 && cluster < FAT32_EOC) {
        uint32_t lba = fs->data_start_lba + (cluster - 2) * fs->sectors_per_cluster;

        for (uint32_t s = 0; s < fs->sectors_per_cluster; s++) {
            uint8_t sector[ATA_SECTOR_SIZE];
            if (ata_read(lba + s, 1, sector) != 0) {
                kprintf("fat32: dir read failed at LBA %d\n", (int)(lba + s));
                return;
            }

            struct fat32_dirent* e = (struct fat32_dirent*)sector;
            for (uint32_t i = 0; i < ATA_SECTOR_SIZE / 32; i++, e++) {
                if (e->name[0] == DIRENT_END) { kprintf("fat32: (end of directory)\n"); return; }
                if (e->name[0] == DIRENT_DELETED) continue;
                if (e->attr == FAT_ATTR_LFN)      continue;
                if (e->attr & FAT_ATTR_VOLUME_ID) continue;

                char pretty[13];
                fat32_format_name(e->name, pretty);
                uint32_t start = ((uint32_t)e->cluster_hi << 16) | e->cluster_lo;
                kprintf("   %s   (cluster %d, %d bytes%s)\n",
                        pretty, (int)start, (int)e->size,
                        (e->attr & FAT_ATTR_DIRECTORY) ? ", dir" : "");
            }
        }

        cluster = fat32_next_cluster(fs, cluster);
        if (++guard > 64) { kprintf("fat32: dir chain guard hit\n"); return; }
    }
}

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

uint32_t fat32_next_cluster(const struct fat32_fs* fs, uint32_t cluster) {
    // Each FAT32 entry is 4 bytes; cluster N's entry is at byte offset N*4.
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->fat_start_lba + (fat_offset / ATA_SECTOR_SIZE);
    uint32_t offset_in_sector = fat_offset % ATA_SECTOR_SIZE;

    uint8_t sector[ATA_SECTOR_SIZE];
    if (ata_read(fat_sector, 1, sector) != 0) {
        kprintf("fat32: FAT read failed at LBA %d\n", (int)fat_sector);
        return 0;
    }

    // Little-endian 32-bit entry; only the low 28 bits are the cluster number.
    uint32_t entry = (uint32_t)sector[offset_in_sector]
                   | ((uint32_t)sector[offset_in_sector + 1] << 8)
                   | ((uint32_t)sector[offset_in_sector + 2] << 16)
                   | ((uint32_t)sector[offset_in_sector + 3] << 24);
    return entry & 0x0FFFFFFF;
}

// Minimal string compare (kernel has no libc). Returns 1 if equal.
static int fat_streq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;   // both must end together
}

int fat32_find(const struct fat32_fs* fs, const char* name, struct fat32_file* out) {
    uint32_t cluster = fs->root_cluster;
    int guard = 0;

    while (cluster >= 2 && cluster < FAT32_EOC) {
        uint32_t lba = fs->data_start_lba + (cluster - 2) * fs->sectors_per_cluster;

        for (uint32_t s = 0; s < fs->sectors_per_cluster; s++) {
            uint8_t sector[ATA_SECTOR_SIZE];
            if (ata_read(lba + s, 1, sector) != 0) {
                kprintf("fat32: dir read failed at LBA %d\n", (int)(lba + s));
                return -1;
            }

            struct fat32_dirent* e = (struct fat32_dirent*)sector;
            for (uint32_t i = 0; i < ATA_SECTOR_SIZE / 32; i++, e++) {
                if (e->name[0] == DIRENT_END)     return -1;   // whole dir scanned
                if (e->name[0] == DIRENT_DELETED) continue;
                if (e->attr == FAT_ATTR_LFN)      continue;
                if (e->attr & FAT_ATTR_VOLUME_ID) continue;

                char pretty[13];
                fat32_format_name(e->name, pretty);
                if (fat_streq(pretty, name)) {
                    out->start_cluster = ((uint32_t)e->cluster_hi << 16) | e->cluster_lo;
                    out->size          = e->size;
                    return 0;                                  // found it
                }
            }
        }

        cluster = fat32_next_cluster(fs, cluster);
        if (++guard > 64) return -1;
    }
    return -1;   // not found
}