#include "include/types.h"
#include "include/fat32.h"
#include "include/ata.h"

/* ── FAT32 globals — set by fat32_init() ── */
uint32_t fat_start_lba;       /* sector where FAT table starts */
uint32_t data_start_lba;      /* sector where file data starts */
uint32_t root_cluster;        /* root directory cluster number */
uint8_t  sectors_per_cluster; /* sectors per cluster */
extern void put_char(char c, char color);
/* ── helper: convert cluster number to LBA sector ── */
static uint32_t cluster_to_lba(uint32_t cluster) {
    return data_start_lba + (cluster - 2) * sectors_per_cluster;
}

/* ── helper: read next cluster from FAT table ── */
static uint32_t fat_next_cluster(uint32_t cluster) {
    uint8_t  buf[512];
    uint32_t fat_offset   = cluster * 4;
    uint32_t fat_sector   = fat_start_lba + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;

    if (ata_read_sector(fat_sector, buf))
        return 0x0FFFFFFF;

    uint32_t next = *(uint32_t*)(buf + entry_offset);
    return next & 0x0FFFFFFF;
}

static void print_hex_byte(uint8_t b) {
    char hi = (b >> 4);
    char lo = (b & 0xF);
    hi = (hi < 10) ? ('0' + hi) : ('A' + hi - 10);
    lo = (lo < 10) ? ('0' + lo) : ('A' + lo - 10);
    put_char(hi, 0x0C);
    put_char(lo, 0x0C);
}

/* ── initialize FAT32 ── */
uint8_t fat32_init(uint32_t partition_lba) {
    uint8_t buf[512];

    if (ata_read_sector(partition_lba, buf))
        return 1;

    int i;
    for (i = 0; i < 16; i++) {
        print_hex_byte(buf[i]);
    }

    if (buf[510] != 0x55 || buf[511] != 0xAA)
        return 2;

    FAT32_Boot *boot = (FAT32_Boot*)buf;

    sectors_per_cluster = boot->sectors_per_cluster;
    root_cluster        = boot->root_cluster;
    fat_start_lba       = partition_lba + boot->reserved_sectors;
    data_start_lba      = fat_start_lba + (boot->fat_count * boot->fat_size_32);

    return 0;
}

/* ── compare 8.3 filename — case insensitive ── */
static uint8_t name_match(uint8_t *entry_name, uint8_t *entry_ext,
                           const char *name, const char *ext) {
    int i;

    /* build padded 8-char name from input */
    uint8_t padded_name[8];
    uint8_t padded_ext[3];

    for (i = 0; i < 8; i++) {
        if (name[i] == '\0') {
            /* fill rest with spaces */
            int j;
            for (j = i; j < 8; j++) padded_name[j] = ' ';
            break;
        }
        uint8_t c = (uint8_t)name[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        padded_name[i] = c;
        if (i == 7) break;
    }

    for (i = 0; i < 3; i++) {
        if (ext[i] == '\0') {
            int j;
            for (j = i; j < 3; j++) padded_ext[j] = ' ';
            break;
        }
        uint8_t c = (uint8_t)ext[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        padded_ext[i] = c;
    }

    /* now compare */
    for (i = 0; i < 8; i++) {
        uint8_t actual = entry_name[i];
        if (actual >= 'a' && actual <= 'z') actual -= 32;
        if (actual != padded_name[i]) return 0;
    }
    for (i = 0; i < 3; i++) {
        uint8_t actual = entry_ext[i];
        if (actual >= 'a' && actual <= 'z') actual -= 32;
        if (actual != padded_ext[i]) return 0;
    }

    return 1;
}
/* ── find file in root directory ── */
uint8_t fat32_find_file(const char *name, const char *ext, FAT32_Entry *out) {
    uint8_t  buf[512];
    uint32_t cluster = root_cluster;
    uint32_t lba;
    int      i, s;

    extern void put_char(char c, char color);

    while (cluster < 0x0FFFFFF8) {
    lba = cluster_to_lba(cluster);
    int end_of_dir = 0;          /* ADD THIS */

    for (s = 0; s < sectors_per_cluster; s++) {
        if (ata_read_sector(lba + s, buf))
            return 1;

        for (i = 0; i < 16; i++) {
            FAT32_Entry *e = (FAT32_Entry*)(buf + i * 32);

            if (e->name[0] == 0x00) { end_of_dir = 1; break; }
            if (e->name[0] == 0xE5) continue;
            if (e->attributes == 0x0F) continue;
            if (e->attributes & 0x10) continue;
            if (e->attributes & 0x08) continue;

            if (name_match(e->name, e->ext, name, ext)) {
                *out = *e;
                return 0;
            }
            

}
} 
}       
}

/* ── read file contents into buffer ── */
uint8_t fat32_read_test() {
    uint8_t buf[512];
    extern void put_char(char c, char color);

    /* read root directory sector directly */
    if (ata_read_sector(4066, buf))
        return 1;

    /* TEST.TXT is at offset 0x40 = entry 2 */
    FAT32_Entry *e = (FAT32_Entry*)(buf + 0x40);

    /* get cluster number */
    uint32_t cluster = ((uint32_t)e->cluster_high << 16) | e->cluster_low;

    /* cluster to LBA — data_start_lba + (cluster-2) * sectors_per_cluster */
    uint32_t lba = cluster_to_lba(cluster);  /* rough calculation */

    /* read file data */
    uint8_t file_buf[512];
    if (ata_read_sector(lba, file_buf))
        return 1;

    /* print first 16 chars */
    int i;
    for (i = 0; i < 16; i++)
        put_char(file_buf[i], 0x0A);

    return 0;
}