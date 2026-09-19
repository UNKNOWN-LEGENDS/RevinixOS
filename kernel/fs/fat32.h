#ifndef FAT32_H
#define FAT32_H
#include <stdint.h>

#define FAT32_EOC 0x0FFFFFF8

// The parts of the FAT32 boot sector (BPB) we care about. Packed: this maps
// directly onto the on-disk bytes of sector 0.
struct fat32_bpb {
    uint8_t  jmp[3];
    uint8_t  oem[8];
    uint16_t bytes_per_sector;     // offset 11
    uint8_t  sectors_per_cluster;  // offset 13
    uint16_t reserved_sectors;     // offset 14  -> FAT region starts here
    uint8_t  num_fats;             // offset 16
    uint16_t root_entry_count;     // offset 17  (0 on FAT32)
    uint16_t total_sectors_16;     // offset 19
    uint8_t  media;                // offset 21
    uint16_t fat_size_16;          // offset 22  (0 on FAT32)
    uint16_t sectors_per_track;    // offset 24
    uint16_t num_heads;            // offset 26
    uint32_t hidden_sectors;       // offset 28
    uint32_t total_sectors_32;     // offset 32
    // FAT32-specific fields follow:
    uint32_t fat_size_32;          // offset 36  sectors per FAT
    uint16_t ext_flags;            // offset 40
    uint16_t fs_version;           // offset 42
    uint32_t root_cluster;         // offset 44  first cluster of root dir (usually 2)
    uint16_t fs_info;              // offset 48
    uint16_t backup_boot_sector;   // offset 50
    uint8_t  reserved[12];         // offset 52
    uint8_t  drive_number;         // offset 64
    uint8_t  reserved1;            // offset 65
    uint8_t  boot_signature;       // offset 66
    uint32_t volume_id;            // offset 67
    uint8_t  volume_label[11];     // offset 71
    uint8_t  fs_type[8];           // offset 82  "FAT32   "
} __attribute__((packed));

// Computed filesystem geometry, filled in by fat32_init().
struct fat32_fs {
    uint32_t fat_start_lba;        // reserved_sectors
    uint32_t data_start_lba;       // fat_start + num_fats*fat_size_32
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_sector;
    uint32_t root_cluster;
};

uint32_t fat32_next_cluster(const struct fat32_fs* fs, uint32_t cluster);

// One 32-byte on-disk directory entry. Packed to match the disk layout.
struct fat32_dirent {
    uint8_t  name[11];        // 8.3, space-padded, no dot (offset 0)
    uint8_t  attr;            // offset 11
    uint8_t  nt_reserved;     // offset 12
    uint8_t  create_tenth;    // offset 13
    uint16_t create_time;     // offset 14
    uint16_t create_date;     // offset 16
    uint16_t access_date;     // offset 18
    uint16_t cluster_hi;      // offset 20  high 16 bits of start cluster
    uint16_t write_time;      // offset 22
    uint16_t write_date;      // offset 24
    uint16_t cluster_lo;      // offset 26  low 16 bits of start cluster
    uint32_t size;            // offset 28  file size in bytes
} __attribute__((packed));

#define FAT_ATTR_LFN         0x0F   // long-filename fragment: skip
#define FAT_ATTR_DIRECTORY   0x10
#define FAT_ATTR_VOLUME_ID   0x08

#define DIRENT_END      0x00   // name[0]: no more entries in this directory
#define DIRENT_DELETED  0xE5   // name[0]: deleted entry, skip

// Debug: read the root directory and print every 8.3 filename found.
void fat32_list_root(const struct fat32_fs* fs);

// Result of a successful directory lookup.
struct fat32_file {
    uint32_t start_cluster;   // first cluster of the file's data
    uint32_t size;            // exact file size in bytes
};

// Find an 8.3 file by name (uppercase "NAME.EXT") in the root directory.
// Returns 0 and fills *out on success; -1 if not found.
int fat32_find(const struct fat32_fs* fs, const char* name, struct fat32_file* out);

// Read + validate the boot sector, fill in geometry. Returns 0 on success.
int fat32_init(struct fat32_fs* fs);

#endif