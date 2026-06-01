    #ifndef ATA_H
    #define ATA_H

    #include "types.h"

    /* Secondary bus ports */
    #define ATA2_DATA        0x170
    #define ATA2_SECTOR_COUNT 0x172
    #define ATA2_LBA_LOW     0x173
    #define ATA2_LBA_MID     0x174
    #define ATA2_LBA_HIGH    0x175
    #define ATA2_DRIVE_HEAD  0x176
    #define ATA2_STATUS      0x177
    #define ATA2_COMMAND     0x177
    #define ATA2_ALT_STATUS  0x376

    /* ATA Primary Bus Ports */
    #define ATA_DATA        0x1F0  /* 16-bit data port */
    #define ATA_ERROR       0x1F1  /* error register */
    #define ATA_SECTOR_COUNT 0x1F2 /* number of sectors to transfer */
    #define ATA_LBA_LOW     0x1F3  /* LBA bits 0-7 */
    #define ATA_LBA_MID     0x1F4  /* LBA bits 8-15 */
    #define ATA_LBA_HIGH    0x1F5  /* LBA bits 16-23 */
    #define ATA_DRIVE_HEAD  0x1F6  /* drive select + LBA bits 24-27 */
    #define ATA_STATUS      0x1F7  /* status register (read) */
    #define ATA_COMMAND     0x1F7  /* command register (write) */
    #define ATA_ALT_STATUS  0x3F6  /* alternate status register */

    /* Status Register Bits */
    #define ATA_SR_BSY      0x80   /* drive busy */
    #define ATA_SR_DRDY     0x40   /* drive ready */
    #define ATA_SR_DRQ      0x08   /* data request ready */
    #define ATA_SR_ERR      0x01   /* error occurred */

    /* ATA Commands */
    #define ATA_CMD_READ    0x20   /* read sectors */
    #define ATA_CMD_WRITE   0x30   /* write sectors */
    #define ATA_CMD_IDENTIFY 0xEC  /* identify drive */

    /* function declarations */
    uint8_t ata_init(void);
    uint8_t ata_read_sector(uint32_t lba, uint8_t *buf);
    uint8_t ata_write_sector(uint32_t lba, uint8_t *buf);

    #endif