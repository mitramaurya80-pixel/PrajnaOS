#ifndef FAT32_H
#define FAT32_H

#include "types.h"

/* ── FAT32 Boot Sector structure ── */
/* sits at sector 0 of the partition */
typedef struct {
    uint8_t  jump[3];           /* jump instruction — ignore */
    uint8_t  oem[8];            /* OEM name — ignore */
    uint16_t bytes_per_sector;  /* usually 512 */
    uint8_t  sectors_per_cluster; /* how many sectors = 1 cluster */
    uint16_t reserved_sectors;  /* sectors before FAT table */
    uint8_t  fat_count;         /* number of FAT copies — usually 2 */
    uint16_t root_entry_count;  /* 0 for FAT32 */
    uint16_t total_sectors_16;  /* 0 for FAT32 */
    uint8_t  media_type;        /* ignore */
    uint16_t fat_size_16;       /* 0 for FAT32 */
    uint16_t sectors_per_track; /* ignore */
    uint16_t head_count;        /* ignore */
    uint32_t hidden_sectors;    /* sectors before partition */
    uint32_t total_sectors_32;  /* total sectors on disk */
    uint32_t fat_size_32;       /* sectors per FAT table */
    uint16_t flags;             /* ignore */
    uint16_t version;           /* ignore */
    uint32_t root_cluster;      /* cluster number of root directory */
    uint16_t fs_info_sector;    /* ignore */
    uint8_t  reserved[464];     /* padding to fill 512 bytes */
} __attribute__((packed)) FAT32_Boot;

/* ── FAT32 Directory Entry ── */
/* each file/folder on disk has one of these — 32 bytes */
typedef struct {
    uint8_t  name[8];           /* filename — padded with spaces */
    uint8_t  ext[3];            /* extension — padded with spaces */
    uint8_t  attributes;        /* file type flags */
    uint8_t  reserved;          /* ignore */
    uint8_t  create_time_fine;  /* ignore */
    uint16_t create_time;       /* ignore */
    uint16_t create_date;       /* ignore */
    uint16_t access_date;       /* ignore */
    uint16_t cluster_high;      /* high 16 bits of start cluster */
    uint16_t modify_time;       /* ignore */
    uint16_t modify_date;       /* ignore */
    uint16_t cluster_low;       /* low 16 bits of start cluster */
    uint32_t file_size;         /* file size in bytes */
} __attribute__((packed)) FAT32_Entry;

/* ── FAT32 globals ── */
/* these are set during fat32_init() and used by all functions */
extern uint32_t fat_start_lba;    /* LBA of FAT table */
extern uint32_t data_start_lba;   /* LBA of data region */
extern uint32_t root_cluster;     /* cluster of root directory */
extern uint8_t  sectors_per_cluster; /* sectors in one cluster */

/* ── function declarations ── */
uint8_t fat32_init(uint32_t partition_lba); /* init — read boot sector */
uint8_t fat32_find_file(const char *name, const char *ext, FAT32_Entry *out); /* find file in root */
uint8_t fat32_read_file(FAT32_Entry *entry, uint8_t *buf, uint32_t max_bytes); /* read file contents */

#endif