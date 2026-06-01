#include "include/types.h"
#include "include/ata.h"

/* ── port I/O functions ── */

/* read a byte from I/O port */
static uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* read a 16-bit word from I/O port */
static uint16_t inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* write a byte to I/O port */
static void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* write a 16-bit word to I/O port */
static void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* ── wait functions ── */

/* wait 400ns — read alt status 4 times */
static void ata_delay(void) {
    inb(ATA_ALT_STATUS);   /* read 1 */
    inb(ATA_ALT_STATUS);   /* read 2 */
    inb(ATA_ALT_STATUS);   /* read 3 */
    inb(ATA_ALT_STATUS);   /* read 4 */
}

/* poll until BSY clears and DRQ sets
   returns 0 on success, 1 on error */
static uint8_t ata_poll(void) {
    uint8_t status;
    uint32_t timeout = 100000;

     while (timeout > 0) {
        status = inb(ATA_STATUS);        /* ← assign status here */
        if (!(status & ATA_SR_BSY)) break;
        timeout--;
    }

    if (status & ATA_SR_ERR) return 1;  /* error */

    /* wait for DRQ */
    timeout = 100000;
    while (timeout > 0) {
        status = inb(ATA_STATUS);
        if (status & ATA_SR_DRQ) return 0;  /* success! */
        if (status & ATA_SR_ERR) return 1;  /* error */
        timeout--;
    }

    return 1;  /* timeout */
}

/* ── ATA identify ── */

/* check if drive exists
   returns 1 if drive found, 0 if not */
static uint8_t ata_identify(void) {
    /* step 1 — select master drive (or slave if you want) */
    outb(ATA_DRIVE_HEAD, 0xB0);

    /* step 2 — zero out sector count and LBA regs */
    outb(ATA_SECTOR_COUNT, 0);
    outb(ATA_LBA_LOW,      0);
    outb(ATA_LBA_MID,      0);
    outb(ATA_LBA_HIGH,     0);

    /* step 3 — send IDENTIFY command */
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);

    /* step 3.5 — wait 400ns for drive to respond */
    ata_delay();

    /* step 4 — read status, if 0 no drive exists */
    uint8_t status = inb(ATA_STATUS);
    if (status == 0x00) return 0;   /* no drive */

    /* step 5 — poll until BSY clears */
    uint32_t timeout = 100000;
    while (timeout > 0 && (inb(ATA_STATUS) & ATA_SR_BSY)) {
        timeout--;
    }

    /* step 6 — check if ATAPI not ATA */
    if (inb(ATA_LBA_MID) != 0 || inb(ATA_LBA_HIGH) != 0)
        return 0;   /* ATAPI device — not plain ATA */

    /* step 7 — wait for DRQ or ERR */
    timeout = 100000;
    while (timeout > 0 && !(inb(ATA_STATUS) & (ATA_SR_DRQ | ATA_SR_ERR))) {
        timeout--;
    }

    /* step 8 — read 256 words (discard for now) */
    for (int i = 0; i < 256; i++)
        inw(ATA_DATA);

    return 1;   /* drive found */
}

/* ── initialize ATA ── */

uint8_t ata_init(void) {
    __asm__ volatile ("cli");  /* disable interrupts during init */
   /* select master drive first */
    // outb(ATA_DRIVE_HEAD, 0xA0);
    
    // /* wait for drive to respond — read alt status 4 times */
    // ata_delay();
    // ata_delay();
    // ata_delay();
    // ata_delay();
    /* disable ATA IRQ */
    // outb(0x3F6, 0x02);
    
    /* select master drive */
    // outb(ATA_DRIVE_HEAD, 0xA0);
    
    uint8_t result = ata_identify();
    __asm__ volatile ("sti");  /* re-enable interrupts */
    /* check if drive exists */
    return result;

}

/* ── read one 512-byte sector ── */

/* lba = sector number to read
   buf = buffer to store data (must be 512 bytes) */
uint8_t ata_read_sector(uint32_t lba, uint8_t *buf) {
     uint32_t timeout = 100000;

    /* wait for BSY to clear BEFORE sending command */
    while (timeout-- && (inb(ATA_STATUS) & ATA_SR_BSY));

    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    ata_delay();

    /* wait again after drive select */
    timeout = 100000;
    while (timeout-- && (inb(ATA_STATUS) & ATA_SR_BSY));

    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_LBA_LOW,  (lba)       & 0xFF);
    outb(ATA_LBA_MID,  (lba >> 8)  & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, ATA_CMD_READ);

    if (ata_poll()) return 1;

    for (int i = 0; i < 256; i++) {
        uint16_t word = inw(ATA_DATA);
        buf[i * 2]     = word & 0xFF;
        buf[i * 2 + 1] = (word >> 8) & 0xFF;
    }

    return 0; /* success */
}

/* ── write one 512-byte sector ── */

/* lba = sector number to write
   buf = data to write (must be 512 bytes) */
uint8_t ata_write_sector(uint32_t lba, uint8_t *buf) {
    uint32_t timeout = 100000;

    /* step 0 — wait for BSY to clear before doing anything */
    while (timeout-- && (inb(ATA_STATUS) & ATA_SR_BSY));

    /* step 1 — select master drive + top 4 LBA bits */
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));

    /* step 2 — send 400ns delay */
    ata_delay();

    /* step 2.5 — wait for BSY again after drive select */
    timeout = 100000;
    while (timeout-- && (inb(ATA_STATUS) & ATA_SR_BSY));

    /* step 3 — send sector count = 1 */
    outb(ATA_SECTOR_COUNT, 1);

    /* step 4 — send LBA address */
    outb(ATA_LBA_LOW,  (lba)       & 0xFF);
    outb(ATA_LBA_MID,  (lba >> 8)  & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);

    /* step 5 — send WRITE SECTORS command */
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    /* step 6 — poll until DRQ ready */
    if (ata_poll()) return 1;

    /* step 7 — write 256 x 16-bit words = 512 bytes */
    for (int i = 0; i < 256; i++) {
        uint16_t word = buf[i * 2] | ((uint16_t)buf[i * 2 + 1] << 8);
        outw(ATA_DATA, word);
    }

    /* step 8 — flush cache to disk */
    outb(ATA_COMMAND, 0xE7);

    /* step 9 — wait for flush to complete */
    timeout = 100000;
    while (timeout-- && (inb(ATA_STATUS) & ATA_SR_BSY));

    return 0;   /* success */
}