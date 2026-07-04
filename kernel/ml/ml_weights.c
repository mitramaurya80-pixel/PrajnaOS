#include "ml_weights.h"
#include "../include/fat32.h"

/* global weight storage — lives in kernel BSS */
float weights_l1[L1_OUT][L1_IN];
float bias_l1[L1_OUT];
float weights_l2[L2_OUT][L2_IN];
float bias_l2[L2_OUT];
float weights_l3[L3_OUT][L3_IN];
float bias_l3[L3_OUT];

uint8_t ml_weights_load(void) {
    /* read weights.bin from FAT32 disk */
    FAT32_Entry entry;
    uint32_t dir_sector;
    uint32_t dir_offset;
    if (fat32_find_file("WEIGHTS", "BIN", &entry,&dir_sector, &dir_offset))
        return 1;  /* file not found */

    /* read entire file into buffer */
    uint8_t buf[512];
    if (fat32_read_file(&entry, buf, 512))
        return 2;  /* read error */

    /* parse header */
    ML_Header *hdr = (ML_Header*)buf;
    if (hdr->magic != ML_MAGIC) return 3;  /* wrong file */

    /* parse weights — skip header (12 bytes) */
    float *p = (float*)(buf + 12);

    /* layer 1: skip layer header (8 bytes) */
    p = (float*)((uint8_t*)p + 8);
    int i, j;
    for (i = 0; i < L1_OUT; i++)
        for (j = 0; j < L1_IN; j++)
            weights_l1[i][j] = *p++;
    for (i = 0; i < L1_OUT; i++)
        bias_l1[i] = *p++;

    /* layer 2: skip layer header */
    p = (float*)((uint8_t*)p + 8);
    for (i = 0; i < L2_OUT; i++)
        for (j = 0; j < L2_IN; j++)
            weights_l2[i][j] = *p++;
    for (i = 0; i < L2_OUT; i++)
        bias_l2[i] = *p++;

    /* layer 3: skip layer header */
    p = (float*)((uint8_t*)p + 8);
    for (i = 0; i < L3_OUT; i++)
        for (j = 0; j < L3_IN; j++)
            weights_l3[i][j] = *p++;
    for (i = 0; i < L3_OUT; i++)
        bias_l3[i] = *p++;

    return 0;  /* success */
}