#include "include/types.h"
#include "include/pit.h"
#include "include/isrc.h"
#include "include/scheduler.h"
#include "include/klib.h"
#include "include/ai_kernel.h"
#include "include/pmm.h"
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
static void print(const char *msg, int row, int col, uint8_t color) {
    char *vga = (char *)0xB8000;
    int col_pos = col;
    for (int i = 0; msg[i] != '\0'; i++) {
        if (msg[i] == '\n') { row++; col_pos = 0; continue; }
        if (msg[i] == '\t') { col_pos += 4; continue; }
        int index = (row * 80 + col_pos) * 2;
        vga[index]     = msg[i];
        vga[index + 1] = color;
        col_pos++;
    }
}
void pit_init(uint32_t hz) {
    uint32_t divisor = PIT_BASE_HZ / hz; /* calculate divisor */

    /* command byte — channel 0, lobyte/hibyte, square wave */
    outb(PIT_COMMAND, 0x36);

    /* send divisor low byte first then high byte */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));         /* low byte */
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF)); /* high byte */
}
/* ── helper — print a fixed-width field, clears old value first ── */
static void print_field(const char *label, const char *value, int row, int col, int width, uint8_t color) {
    /* clear the zone first — write spaces */
    char blank[32];
    int i;
    for (i = 0; i < width && i < 31; i++) blank[i] = ' ';
    blank[i] = '\0';
    print(blank, row, col, 0x07);

    /* print label + value */
    print(label, row, col, 0x08);           /* dim label */
    print(value, row, col + kstrlen(label), color);  /* bright value */
}
/* draw horizontal divider across full 80 columns */
static void draw_divider(int row, uint8_t color) {
    for (int i = 0; i < 80; i++)
        print("\xC4", row, i, color);
}
void draw_top_bar(void) {
    char buf[16];
    int i;

    /* clear row 0 with dark background */
    for (i = 0; i < 80; i++)
        print(" ", 0, i, 0x17);   /* 0x17 = white text on blue bg */

    /* title — left */
    print("PrajnaOS", 0, 1, 0x1F);   /* bright white on blue */

    /* separator */
    print("\xB3", 0, 10, 0x17);   /* │ */

    /* state */
    sys_state_t st = ai_get_state();
    char *state_str = (st == STATE_CALM)   ? "CALM  " :
                      (st == STATE_NORMAL) ? "NORMAL" : "ALERT ";
    uint8_t sc = (st == STATE_ALERT) ? 0x1C :   /* red on blue */
                 (st == STATE_NORMAL) ? 0x1E :   /* yellow on blue */
                                        0x1A;    /* green on blue */
    print(state_str, 0, 12, sc);

    /* separator */
    print("\xB3", 0, 19, 0x17);

    /* memory */
    uint32_t pages = pmm_free_pages();
    uint_to_str(pages, buf);
    print("Mem:", 0, 21, 0x17);
    print(buf,    0, 25, 0x1F);
    print("pg",  0, 25 + kstrlen(buf), 0x17);

    /* separator */
    print("\xB3", 0, 43, 0x17);

    /* uptime */
    uint_to_str(ticks, buf);
    uint32_t uptime = ticks / 100;  /* convert ticks to seconds */
    uint_to_str(uptime, buf);
    print("Up:", 0, 45, 0x17);
    print(buf,   0, 48, 0x1F);
    print("s",   0, 48 + kstrlen(buf), 0x17);

    /* separator */
    print("\xB3", 0, 62, 0x17);

    /* tasks */
    uint32_t count = 0;
    for (i = 0; i < MAX_TASKS; i++)
        if (tasks[i].state != TASK_DEAD) count++;
    uint_to_str(count, buf);
    print("Tasks:", 0, 64, 0x17);
    print(buf,      0, 70, 0x1F);

    /* divider line row 1 */
    draw_divider(1, 0x08);
}

/* called every timer interrupt from idt.c */
void pit_handler(void) {
    ticks++;   /* increment tick counter */

    if (ticks % 10 == 0) { /* every 10 ticks (1 second) */
        blink(); /* call blink function to toggle LED */
    }
    if (ticks % 100 == 0) {

    scheduler_tick();  /* call scheduler tick to switch tasks */
    }
    if (ticks % 100 == 0) { /* every 100 ticks (1 second) */
        draw_top_bar(); /* update top bar with state/mem/tasks/uptime */
    }
}

/* return current tick count */
uint32_t pit_get_ticks(void) {
    return ticks;
}
