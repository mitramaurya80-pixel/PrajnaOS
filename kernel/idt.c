#include "include/idt.h" // IDT header file
#include "include/pit.h"

struct idt_entry idt[256]; // IDT entry array
struct idt_ptr   ip; // IDT pointer

extern void idt_flush(uint32_t); // IDT flush function
extern void isr_keyboard(); // Keyboard interrupt handler
extern void isr_timer();       // Timer interrupt handler
extern void isr_default();     // Default interrupt handler

static void outb(uint16_t port, uint8_t val) { // Output byte to port
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); // Output byte to port
}

static void idt_set(uint8_t i, uint32_t base, uint16_t sel, uint8_t flags) { // Set IDT entry
    idt[i].base_low  = base & 0xFFFF; // Set base low
    idt[i].base_high = (base >> 16) & 0xFFFF; // Set base high
    idt[i].selector  = sel; // Set selector
    idt[i].zero      = 0; // Set zero to 0 (reserved by Intel CPU design)
    idt[i].flags     = flags; // Set flags
}
void irq0_handler(void) {
    pit_handler();   /* increment tick counter */
}

void idt_init() { // Initialize IDT
    ip.limit = (sizeof(struct idt_entry) * 256) - 1; // Set limit
    ip.base  = (uint32_t)&idt; // Set base
    //
    for (int i = 0; i < 256; i++) // Set all IDT entries to 0
        idt_set(i,(uint32_t)isr_default, 0x08, 0x8E); // Set IDT entry to default handler

    // Remap PIC — shift IRQs to interrupts 32-47
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);  // Master IRQs start at 32
    outb(0xA1, 0x28);  // Slave  IRQs start at 40
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);

    /* override with real handlers */
    idt_set(32, (uint32_t)isr_timer,    0x08, 0x8E);
    idt_set(33, (uint32_t)isr_keyboard, 0x08, 0x8E);
    idt_set(46, (uint32_t)isr_default, 0x08, 0x8E);  /* default handler for unhandled interrupts */

    idt_flush((uint32_t)&ip);
}
