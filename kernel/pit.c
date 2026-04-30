#include "include/types.h"
#include "include/pit.h"
#include "include/isrc.h"

/* PIT hardware ports */
#define PIT_CHANNEL0  0x40   /* channel 0 data port */
#define PIT_COMMAND   0x43   /* command register */
#define PIT_BASE_HZ   1193180 /* PIT base frequency in Hz */

/* tick counter — increments every timer interrupt */
static uint32_t ticks = 0;

/* write a byte to I/O port */
static void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * pit_init() — set PIT to fire IRQ0 at given frequency
 * hz = how many times per second (100 = 100 ticks/sec)
 */
void pit_init(uint32_t hz) {
    uint32_t divisor = PIT_BASE_HZ / hz; /* calculate divisor */

    /* command byte — channel 0, lobyte/hibyte, square wave */
    outb(PIT_COMMAND, 0x36);

    /* send divisor low byte first then high byte */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));         /* low byte */
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF)); /* high byte */
}

/* called every timer interrupt from idt.c */
void pit_handler(void) {
    ticks++;   /* increment tick counter */
    if (ticks % 10 == 0) { /* every 10 ticks (1 second) */
        blink(); /* call blink function to toggle LED */
    }
}

/* return current tick count */
uint32_t pit_get_ticks(void) {
    return ticks;
}