#include "include/types.h"
#include "include/fat32.h"
#include "include/ata.h"

/* ── FAT32 globals — set by fat32_init() ── */
uint32_t fat_start_lba;       /* sector where FAT table starts */
uint32_t data_start_lba;      /* sector where file data starts */
uint32_t root_cluster;        /* root directory cluster number */
uint8_t  sectors_per_cluster; /* sectors per cluster */
uint32_t current_cluster;      /* for directory iteration */
uint32_t previous_cluster;  /* tracks parent directory */
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



/* ── initialize FAT32 ── */
uint8_t fat32_init(uint32_t partition_lba) {
    uint8_t buf[512];

    if (ata_read_sector(partition_lba, buf))
        return 1;

    if (buf[510] != 0x55 || buf[511] != 0xAA)
        return 2;

    FAT32_Boot *boot = (FAT32_Boot*)buf;

    sectors_per_cluster = boot->sectors_per_cluster;
    root_cluster        = boot->root_cluster;
    fat_start_lba       = partition_lba + boot->reserved_sectors;
    data_start_lba      = fat_start_lba + (boot->fat_count * boot->fat_size_32);
    current_cluster     = root_cluster;
    previous_cluster =root_cluster;

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
uint8_t fat32_find_file(const char *name, const char *ext,
                         FAT32_Entry *out,
                         uint32_t *out_sector,
                         uint32_t *out_offset) {
    uint8_t  buf[512];
    uint32_t cluster = current_cluster;
    uint32_t lba;
    int      i, s;

    while (cluster < 0x0FFFFFF8) {
        lba = cluster_to_lba(cluster);
        int end_of_dir = 0;

        for (s = 0; s < sectors_per_cluster; s++) {
            if (ata_read_sector(lba + s, buf)) return 1;

            for (i = 0; i < 16; i++) {
                FAT32_Entry *e = (FAT32_Entry*)(buf + i * 32);

                if (e->name[0] == 0x00) { end_of_dir = 1; break; }
                if (e->name[0] == 0xE5) continue;
                if (e->attributes == 0x0F) continue;
                if (e->attributes & 0x10) continue;
                if (e->attributes & 0x08) continue;

                if (name_match(e->name, e->ext, name, ext)) {
                    *out        = *e;
                    *out_sector = lba + s;      /* which sector */
                    *out_offset = i * 32;       /* byte offset in sector */
                    return 0;
                }
            }
            if (end_of_dir) break;
        }
        if (end_of_dir) break;
        cluster = fat_next_cluster(cluster);
    }
    return 1;
}
/* ── read file contents into buffer ── */
uint8_t fat32_read_file(FAT32_Entry *entry, uint8_t *buf, uint32_t max_bytes) {
    uint32_t cluster = ((uint32_t)entry->cluster_high << 16) | entry->cluster_low;
    uint32_t bytes_written = 0;
    uint32_t safety = 0;  /* prevent infinite loop */

    while (cluster < 0x0FFFFFF8) {
        if (safety++ > 128) break;  /* max 128 clusters */

        uint32_t lba = cluster_to_lba(cluster);

        int s;
        for (s = 0; s < sectors_per_cluster; s++) {
            if (ata_read_sector(lba + s, buf + bytes_written))
                return 1;
            bytes_written += 512;
            if (bytes_written >= max_bytes) return 0;
        }

        cluster = fat_next_cluster(cluster);
    }
    return 0;
}

/* list all files in root directory — for ls command */
void fat32_list_dir() {
    uint8_t  buf[512];
    uint32_t cluster = current_cluster;  /* start from root */
    uint32_t lba;
    int      i, s, n;
    extern void put_char(char c, char color);

    while (cluster < 0x0FFFFFF8) {
        lba = cluster_to_lba(cluster);

        for (s = 0; s < sectors_per_cluster; s++) {
            if (ata_read_sector(lba + s, buf)) return;

            for (i = 0; i < 16; i++) {
                uint8_t *e = buf + i * 32;

                /* end of directory */
                if (e[0] == 0x00) return;

                /* skip deleted, LFN, volume label */
                if (e[0] == 0xE5) continue;
                if (e[11] == 0x0F) continue;
                if (e[11] & 0x08) continue;

                /* print name — 8 chars */
                for (n = 0; n < 8; n++) {
                    if (e[n] == ' ') break;
                    put_char(e[n], 0x0A);  /* green */
                }

                /* if not directory — print extension */
                if (!(e[11] & 0x10)) {
                    put_char('.', 0x0A);
                    for (n = 8; n < 11; n++) {
                        if (e[n] == ' ') break;
                        put_char(e[n], 0x0A);
                    }
                } else {
                    /* directory — print / */
                    put_char('/', 0x0B);  /* cyan */
                }

                put_char('\n', 0x0A);
            }
        }
        cluster = fat_next_cluster(cluster);
    }
}
static void fat_set_cluster(uint32_t cluster, uint32_t value) {
    uint8_t  buf[512];
    uint32_t fat_offset   = cluster * 4;
    uint32_t fat_sector   = fat_start_lba + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;

    /* read FAT sector */
    if (ata_read_sector(fat_sector, buf)) return;

    /* write new value */
    *(uint32_t*)(buf + entry_offset) = value & 0x0FFFFFFF;

    /* write back */
    ata_write_sector(fat_sector, buf);
}
static uint32_t fat32_find_free_cluster() {
    uint8_t  buf[512];
    uint32_t cluster;

    /* scan FAT table looking for 0x00000000 = free */
    for (cluster = 2; cluster < 0x0FFFFFF8; cluster++) {
        uint32_t fat_offset   = cluster * 4;
        uint32_t fat_sector   = fat_start_lba + (fat_offset / 512);
        uint32_t entry_offset = fat_offset % 512;

        if (ata_read_sector(fat_sector, buf)) return 0;

        uint32_t val = *(uint32_t*)(buf + entry_offset) & 0x0FFFFFFF;
        if (val == 0x00000000)
            return cluster;  /* free cluster found */
    }
    return 0;  /* disk full */
}
uint8_t fat32_create_file(const char *name, const char *ext) {
    uint8_t buf[512];
    uint32_t lba = cluster_to_lba(current_cluster);
    int i, n;

    /* read root directory sector */
    if (ata_read_sector(lba, buf)) return 1;

    /* find empty entry — name[0] == 0x00 or 0xE5 */
    for (i = 0; i < 16; i++) {
        uint8_t *e = buf + i * 32;
        if (e[0] == 0x00 || e[0] == 0xE5) {

            /* clear entry */
            for (n = 0; n < 32; n++) e[n] = 0;

            /* write name — 8 chars space padded */
            for (n = 0; n < 8; n++) {
                if (name[n] == '\0') break;
                uint8_t c = name[n];
                if (c >= 'a' && c <= 'z') c -= 32;
                e[n] = c;
            }
            for (; n < 8; n++) e[n] = ' ';

            /* write ext — 3 chars space padded */
            for (n = 0; n < 3; n++) {
                if (ext[n] == '\0') break;
                uint8_t c = ext[n];
                if (c >= 'a' && c <= 'z') c -= 32;
                e[8 + n] = c;
            }
            for (; n < 3; n++) e[8 + n] = ' ';

            /* attributes — normal file */
            e[11] = 0x20;

            /* allocate cluster */
            uint32_t cluster = fat32_find_free_cluster();
            if (cluster == 0) return 2;  /* disk full */

            /* mark cluster as end of chain */
            fat_set_cluster(cluster, 0x0FFFFFFF);

            /* store cluster in entry */
            e[20] = (cluster >> 16) & 0xFF;  /* cluster high low byte */
            e[21] = (cluster >> 24) & 0xFF;  /* cluster high high byte */
            e[26] = (cluster) & 0xFF;         /* cluster low low byte */
            e[27] = (cluster >> 8) & 0xFF;   /* cluster low high byte */

            /* file size = 0 for empty file */
            e[28] = 0; e[29] = 0; e[30] = 0; e[31] = 0;

            /* write sector back */
            ata_write_sector(lba, buf);
            return 0;  /* success */
        }
    }
    return 3;  /* directory full */
}
uint8_t fat32_find_dir(const char *name, FAT32_Entry *out) {
    uint8_t  buf[512];
    uint32_t cluster = current_cluster;
    uint32_t lba;
    int      i, s;

    while (cluster < 0x0FFFFFF8) {
        lba = cluster_to_lba(cluster);

        for (s = 0; s < sectors_per_cluster; s++) {
            if (ata_read_sector(lba + s, buf)) return 1;

            for (i = 0; i < 16; i++) {
                uint8_t *e = buf + i * 32;

                if (e[0] == 0x00) return 2;
                if (e[0] == 0xE5) continue;
                if (e[11] == 0x0F) continue;

                /* only match directories */
                if (!(e[11] & 0x10)) continue;

                /* match name only — no extension for directories */
                uint8_t match = 1;
                int m;
                for (m = 0; m < 8; m++) {
                    uint8_t disk = e[m];
                    if (disk >= 'a' && disk <= 'z') disk -= 32;
                    uint8_t want = name[m] ? (uint8_t)name[m] : ' ';
                    if (want >= 'a' && want <= 'z') want -= 32;
                    if (disk != want) { match = 0; break; }
                }

                if (match) {
                    uint8_t *dst = (uint8_t*)out;
                    for (m = 0; m < 32; m++) dst[m] = e[m];
                    return 0;
                }
            }
        }
        cluster = fat_next_cluster(cluster);
    }
    return 2;
}
uint8_t fat32_write_file(FAT32_Entry *entry, uint32_t dir_sector,
                          uint32_t dir_offset, uint8_t *data, uint32_t size) {
    uint8_t sector_buf[512];
    uint32_t i;
    uint32_t bytes_written = 0;
    /* get cluster from entry */
    uint32_t cluster = ((uint32_t)entry->cluster_high << 16) | entry->cluster_low;
    uint32_t prev_cluster = 0;

    /* if no cluster allocated yet — find a free one */
    if (cluster == 0) {
        cluster = fat32_find_free_cluster();
        if (cluster == 0) return 1;  /* disk full */
        fat_set_cluster(cluster, 0x0FFFFFFF);  /* end of chain */

        /* update directory entry with new cluster */
        entry->cluster_high = (cluster >> 16) & 0xFFFF;
        entry->cluster_low  = cluster & 0xFFFF;
    }
    while(bytes_written < size) {
        /* get LBA of cluster */
        uint32_t lba = cluster_to_lba(cluster);
        uint32_t chunk = (size - bytes_written) < 512 ? (size - bytes_written) : 512;   

        /* clear sector buffer */
        for (i = 0; i < 512; i++) sector_buf[i] = 0;

        for (i = 0; i < chunk; i++)
            sector_buf[i] = data[bytes_written + i];

        /* write sector to disk */
        if (ata_write_sector(lba, sector_buf)) return 2;

        bytes_written += chunk;

        /* if more data to write — get next cluster */
        if (bytes_written < size) {
            prev_cluster = cluster;
            uint32_t next = fat_next_cluster(cluster);
            if (cluster >= 0x0FFFFFF8) {
                /* allocate new cluster */
                next = fat32_find_free_cluster();
                if (next == 0) return 3;  /* disk full */
                fat_set_cluster(prev_cluster, next);  /* link previous to new */
                fat_set_cluster(next, 0x0FFFFFFF);     /* mark new as end of chain */
                prev_cluster = cluster;
                cluster=next;
            }
        }
        return 0;  /* success */
    }

    entry->file_size = size;

    uint8_t dir_buf[512];
    if (ata_read_sector(dir_sector, dir_buf)) return 3; 
    FAT32_Entry *e = (FAT32_Entry*)(dir_buf + dir_offset);
    e->file_size    = size;
    e->cluster_high = entry->cluster_high;
    e->cluster_low  = entry->cluster_low;
    if (ata_write_sector(dir_sector, dir_buf)) return 4;

    return 0;  /* success */
}
uint8_t fat32_file_delete(const char *name, const char *ext) {
    FAT32_Entry entry;
    uint32_t dir_sector, dir_offset;

    if (fat32_find_file(name, ext, &entry, &dir_sector, &dir_offset) != 0) {
        return -1;  /* file not found */
    }

    /* mark directory entry as deleted */
    uint8_t buf[512];
    if (ata_read_sector(dir_sector, buf)) return -2;  /* read error */

    buf[dir_offset] = 0xE5;  /* mark as deleted */

    if (ata_write_sector(dir_sector, buf)) return -3;  /* write error */

    /* free clusters in FAT */
    uint32_t cluster = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    while (cluster < 0x0FFFFFF8) {
        uint32_t next = fat_next_cluster(cluster);
        fat_set_cluster(cluster, 0x00000000);  /* mark as free */
        cluster = next;
    }

    return 0;  /* success */
}