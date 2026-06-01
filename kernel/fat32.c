#include "include/types.h"
#include "include/fat32.h"
#include "include/ata.h"

/* ── FAT32 globals — set by fat32_init() ── */
uint32_t fat_start_lba;       /* sector where FAT table starts */
uint32_t data_start_lba;      /* sector where file data starts */
uint32_t root_cluster;        /* root directory cluster number */
uint8_t  sectors_per_cluster; /* sectors per cluster */

/* ── helper: convert cluster number to LBA sector ── */
/* every cluster is sectors_per_cluster sectors wide */
static uint32_t cluster_to_lba(uint32_t cluster) {
    /* cluster 2 is the first data cluster */
    return data_start_lba + (cluster - 2) * sectors_per_cluster;
}

/* ── helper: read next cluster from FAT table ── */
/* FAT32 stores a chain — each cluster points to the next */
/* 0x0FFFFFFF means end of file */
static uint32_t fat_next_cluster(uint32_t cluster) {
    uint8_t  buf[512];          /* sector buffer */
    uint32_t fat_offset;        /* byte offset in FAT */
    uint32_t fat_sector;        /* which sector of FAT */
    uint32_t entry_offset;      /* byte offset within that sector */

    /* each FAT32 entry is 4 bytes */
    fat_offset   = cluster * 4;
    fat_sector   = fat_start_lba + (fat_offset / 512);
    entry_offset = fat_offset % 512;

    /* read that FAT sector */
    if (ata_read_sector(fat_sector, buf))
        return 0x0FFFFFFF;  /* read error — treat as end */

    /* read 4 bytes at entry_offset — mask top 4 bits */
    uint32_t next = *(uint32_t*)(buf + entry_offset);
    return next & 0x0FFFFFFF;  /* mask reserved bits */
}
static void print_hex_byte(uint8_t b) {
    extern void put_char(char c, char color);
    char hi = (b >> 4);
    char lo = (b & 0xF);
    hi = (hi < 10) ? ('0' + hi) : ('A' + hi - 10);
    lo = (lo < 10) ? ('0' + lo) : ('A' + lo - 10);
    put_char(hi, 0x0C);
    put_char(lo, 0x0C);
}
/* ── initialize FAT32 ── */
/* partition_lba = sector where the FAT32 partition starts */
/* for your disk.img this is usually 0 or 2048 */
uint8_t fat32_init(uint32_t partition_lba) {
    uint8_t buf[512];

    /* read boot sector of partition */
    if (ata_read_sector(partition_lba, buf))
        return 1;  /* read error */
    int i;
    for (i = 0; i < 16; i++) {
        print_hex_byte(buf[i]);
    }
    if (buf[510] != 0x55 || buf[511] != 0xAA) {
        return 2;  /* not a valid boot sector */
    }

    /* cast buffer to boot sector struct */
    FAT32_Boot *boot = (FAT32_Boot*)buf;

    /* check signature — last 2 bytes must be 0x55 0xAA */
    if (buf[510] != 0x55 || buf[511] != 0xAA)
        return 2;  /* not a valid boot sector */

    /* save important values globally */
    sectors_per_cluster = boot->sectors_per_cluster;
    root_cluster        = boot->root_cluster;

    /* FAT table starts after reserved sectors */
    fat_start_lba = partition_lba + boot->reserved_sectors;

    /* data region starts after all FAT copies */
    data_start_lba = fat_start_lba + (boot->fat_count * boot->fat_size_32);

    return 0;  /* success */
}

/* ── compare 8.3 filename ── */
/* FAT32 stores names as 8 chars + 3 chars, space padded */
/* "IRIS    TXT" not "iris.txt" */
static uint8_t name_match(uint8_t *entry_name, uint8_t *entry_ext,
                           const char *name, const char *ext) {
    int i;

    /* compare name — 8 chars */
    for (i = 0; i < 8; i++) {
        uint8_t actual = entry_name[i];
        if (actual >= 'a' && actual <= 'z') actual -= 32;

        uint8_t expected;
        if ((uint8_t)name[i] == 0)
            expected = ' ';
        else {
            expected = (uint8_t)name[i];
            if (expected >= 'a' && expected <= 'z') expected -= 32;
        }

        if (actual != expected) return 0;
    }

    /* compare ext — 3 chars */
    for (i = 0; i < 3; i++) {
        uint8_t actual = entry_ext[i];
        if (actual >= 'a' && actual <= 'z') actual -= 32;

        uint8_t expected;
        if ((uint8_t)ext[i] == 0)
            expected = ' ';
        else {
            expected = (uint8_t)ext[i];
            if (expected >= 'a' && expected <= 'z') expected -= 32;
        }

        if (actual != expected) return 0;
    }

    return 1;
}
// hex byte print

/* ── find file in root directory ── */
/* name = filename up to 8 chars, ext = extension up to 3 chars */
/* out = filled with directory entry if found */
uint8_t fat32_find_file(const char *name, const char *ext, FAT32_Entry *out) {
    uint8_t  buf[512];
    uint32_t cluster = root_cluster;
    uint32_t lba;
    int      i, s;

    extern void put_char(char c, char color);

    /* debug — print root cluster and data_start_lba */
    // put_char('C', 0x0E);
    // put_char('0' + (root_cluster & 0xF), 0x0E);
    // put_char('D', 0x0E);
    // put_char('0' + (data_start_lba & 0xF), 0x0E);

    while (cluster < 0x0FFFFFF8) {
        lba = cluster_to_lba(cluster);

        for (s = 0; s < sectors_per_cluster; s++) {
            if (ata_read_sector(lba + s, buf))
                return 1;

            for (i = 0; i < 16; i++) {
    FAT32_Entry *e = (FAT32_Entry*)(buf + i * 32);

    if (e->name[0] == 0x00) goto done;
    if (e->name[0] == 0xE5) continue;
    if (e->attributes == 0x0F) continue;
    if (e->attributes & 0x10) continue;
    if (e->attributes & 0x08) continue;

    /* inline match — no debug */
    uint8_t match = 1;
    int m;
    for (m = 0; m < 8; m++) {
        uint8_t a = e->name[m];
        uint8_t b = name[m] ? (uint8_t)name[m] : ' ';
        if (a != b) { match = 0; break; }
    }
    if (match) {
        for (m = 0; m < 3; m++) {
            uint8_t a = e->ext[m];
            uint8_t b = ext[m] ? (uint8_t)ext[m] : ' ';
            if (a != b) { match = 0; break; }
        }
    }
    if (match) {
        *out = *e;
        return 0;
    }
}
        }
        cluster = fat_next_cluster(cluster);
    }

done:
    return 2;  /* not found */
}
/* ── read file contents into buffer ── */
/* entry = directory entry from fat32_find_file() */
/* buf   = output buffer */
/* max_bytes = size of your buffer */
uint8_t fat32_read_file(FAT32_Entry *entry, uint8_t *buf, uint32_t max_bytes) {
    uint8_t  sector_buf[512];   /* one sector at a time */
    uint32_t bytes_read = 0;    /* how many bytes copied so far */
    uint32_t s;

    /* get starting cluster from directory entry */
    uint32_t cluster = ((uint32_t)entry->cluster_high << 16) | entry->cluster_low;

    /* walk cluster chain */
    while (cluster < 0x0FFFFFF8 && bytes_read < max_bytes) {
        uint32_t lba = cluster_to_lba(cluster);

        /* read each sector in cluster */
        for (s = 0; s < sectors_per_cluster && bytes_read < max_bytes; s++) {
            if (ata_read_sector(lba + s, sector_buf))
                return 1;  /* read error */

            /* copy sector to output buffer */
            uint32_t to_copy = 512;
            if (bytes_read + to_copy > max_bytes)
                to_copy = max_bytes - bytes_read;  /* don't overflow */

            uint32_t j;
            for (j = 0; j < to_copy; j++)
                buf[bytes_read + j] = sector_buf[j];

            bytes_read += to_copy;
        }

        /* next cluster */
        cluster = fat_next_cluster(cluster);
    }

    return 0;  /* success */
}