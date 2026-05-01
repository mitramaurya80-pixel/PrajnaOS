#include "include/gdt.h"
#include "include/idt.h"

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
    outb(0x3D4, 0x0A);      // select cursor start register
    outb(0x3D5, 0x20);      // bit 5 = disable cursor
    print("Welcome to PrajnaOS",0, 30, 0x09);  /* row 0, green  */ 
   

    gdt_init();  /* set up memory segments (must come first)  */
    idt_init();  /* set up interrupts + enable keyboard        */
    print("PrajnaOS>",2,0, 0x03);  /* row 1, cyan   */

    while (1) {}
    /* CPU waits here — keyboard interrupt wakes it on each keypress */
}
