#include "include/gdt.h"
#include "include/idt.h"
#include "include/pit.h"
#include "include/ata.h"

static void print(const char *msg, int row, int col, uint8_t color) {
    char *vga = (char *)0xB8000;

    for (int i = 0; msg[i] != '\0'; i++) {
        int index = (row * 80 + (col + i)) * 2;

        vga[index]     = msg[i];   /* character */
        vga[index + 1] = color;    /* color */
    }
}

static void outb(uint16_t port, uint8_t val) { // Output byte to port
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); // Output byte to port
}

void kernel_main() {
    __asm__ volatile ("cli");  /* disable interrupts during init */
    outb(0x3D4, 0x0A);      // select cursor start register
    outb(0x3D5, 0x20);      // bit 5 = disable cursor
    print("Welcome to PrajnaOS",0, 30, 0x09);  /* row 0, green  */ 
   

    gdt_init();  /* set up memory segments (must come first)  */
        
    idt_init();  /* set up interrupts + enable keyboard        */
   
    pit_init(100);  /* set up timer to tick every 100ms            */
    
    ata_init();  /* set up ATA controller                       */
    print("ATA initialized",7,0, 0x02);  /* row 6, light blue  */
    uint8_t buf[512];
    uint8_t res = ata_read_sector(0, buf);  /* read sector 0 (MBR) into buf */
    if(res==0){
        print("Read sector 0 successfully!",8,0, 0x02);  /* row 7, light blue  */
    } else {
        print("Failed to read sector 0",8,0, 0x04);  /* row 7, red   */
    }
    print("PrajnaOS>",2,0, 0x03);  /* row 1, cyan   */

    while (1) {}
    /* CPU waits here — keyboard interrupt wakes it on each keypress */
}
