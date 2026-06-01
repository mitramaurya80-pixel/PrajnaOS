// #include "include/gdt.h"
// #include "include/idt.h"
// #include "include/pit.h"
// #include "include/ata.h"

// static void print(const char *msg, int row, int col, uint8_t color) {
//     char *vga = (char *)0xB8000;

//     for (int i = 0; msg[i] != '\0'; i++) {
//         int index = (row * 80 + (col + i)) * 2;

//         vga[index]     = msg[i];   /* character */
//         vga[index + 1] = color;    /* color */
//     }
// }

// static void outb(uint16_t port, uint8_t val) { // Output byte to port
//     __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); // Output byte to port
// }

// void kernel_main() {
//     __asm__ volatile ("cli");  /* disable interrupts during init */
//     outb(0x3D4, 0x0A);      // select cursor start register
//     outb(0x3D5, 0x20);      // bit 5 = disable cursor
//     print("Welcome to PrajnaOS",0, 30, 0x09);  /* row 0, green  */ 
   

//     gdt_init();  /* set up memory segments (must come first)  */
        
//     idt_init();  /* set up interrupts + enable keyboard        */
   
//     pit_init(100);  /* set up timer to tick every 100ms            */
    
//     ata_init();  /* set up ATA controller                       */
//     print("ATA initialized",7,0, 0x02);  /* row 6, light blue  */
//     uint8_t buf[512];
//     uint8_t res = ata_read_sector(0, buf);  /* read sector 0 (MBR) into buf */
//     if(res==0){
//         print("Read sector 0 successfully!",8,0, 0x02);  /* row 7, light blue  */
//     } else {
//         print("Failed to read sector 0",8,0, 0x04);  /* row 7, red   */
//     }
//     print("PrajnaOS>",2,0, 0x03);  /* row 1, cyan   */
//     /* write test — write "PRAJNA" to sector 1 */
// uint8_t wbuf[512];
// uint8_t rbuf[512];
// int i;

// /* clear buffer */
// for (i = 0; i < 512; i++) wbuf[i] = 0;

// /* write PRAJNA to buffer */
// wbuf[0] = 'P'; wbuf[1] = 'R'; wbuf[2] = 'A';
// wbuf[3] = 'J'; wbuf[4] = 'N'; wbuf[5] = 'A';

// /* write to sector 1 */
// uint8_t wres = ata_write_sector(1, wbuf);

// /* read back sector 1 */
// uint8_t rres = ata_read_sector(1, rbuf);

// /* verify first 6 bytes match */
// if (rres == 0 && rbuf[0] == 'P' && rbuf[1] == 'R')
//     print("Write test PASSED", 10, 0, 0x02);
// else
//     print("Write test FAILED", 10, 0, 0x04);

//     while (1) {}
//     /* CPU waits here — keyboard interrupt wakes it on each keypress */
// }
#include "include/gdt.h"
#include "include/idt.h"
#include "include/pit.h"
#include "include/ata.h"
#include "include/fat32.h"    /* add this */


/* ── must be above kernel_main ── */
static void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void print(const char *msg, int row, int col, uint8_t color) {
    char *vga = (char *)0xB8000;
    for (int i = 0; msg[i] != '\0'; i++) {
        int index = (row * 80 + (col + i)) * 2;
        vga[index]     = msg[i];
        vga[index + 1] = color;
    }
}
void kernel_main() {
     /* enable FPU — needed for float in classify_iris */
    __asm__ volatile (
        "mov %%cr0, %%eax\n"
        "and $0xFFFFFFFB, %%eax\n"  /* clear EM bit */
        "or $0x2, %%eax\n"           /* set MP bit */
        "mov %%eax, %%cr0\n"
        "fninit\n"                   /* initialize FPU */
        : : : "eax"
    );
    __asm__ volatile ("cli");
    outb(0x3D4, 0x0A); // select cursor start register
    outb(0x3D5, 0x20); // bit 5 = disable cursor
    print("Welcome to PrajnaOS", 0, 30, 0x09);

    gdt_init();
    idt_init();
    pit_init(100);
    ata_init();
    print("ATA initialized", 1, 0, 0x02);

    /* ── L4: FAT32 init ── */
    /* try partition at LBA 0 first */
    uint8_t fat_res = fat32_init(2048);  /* for our disk.img, partition starts at LBA 2048 */
    if (fat_res == 0)
        print("FAT32 mounted", 3, 0, 0x02);
    else
        print("FAT32 failed", 3, 0, 0x04);

    /* ── L4: find and read a file ── */
    FAT32_Entry entry;
    uint8_t fat_find = fat32_find_file("TEST", "TXT", &entry);
    if (fat_find == 0) {
        uint8_t file_buf[512];
        fat32_read_file(&entry, file_buf, 512);
        print("File found!", 4, 0, 0x02);
    } else {
        print("File not found", 4, 0, 0x04);
    }

    print("PrajnaOS>", 5, 0, 0x03);
    while (1) {}
}
