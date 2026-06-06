#include "include/pmm.h"
#include "include/gdt.h"
#include "include/idt.h"
#include "include/pit.h"
#include "include/ata.h"
#include "include/fat32.h"
#include "include/ml_math.h"
#include "include/shell.h"
#include "include/scheduler.h"

extern void clear_screen();
extern uint32_t kernel_end;

static void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

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

void wait_ticks(uint32_t ticks) {
    uint32_t start = pit_get_ticks();
    while (pit_get_ticks() - start < ticks) {}
}

void wait_seconds(uint32_t seconds) {
    wait_ticks(seconds * 100);
}

/* ── test tasks — defined OUTSIDE kernel_main ── */
void task1() {
    char *vga = (char*)0xB8000;
    uint32_t i = 0;
    while (1) {
        vga[0] = 'A';
        vga[1] = 0x0A;
        i++;
        if (i == 10000000) {
            i = 0;
            scheduler_tick();  /* manually switch */
        }
    }
}

void task2() {
    char *vga = (char*)0xB8000;
    uint32_t i = 0;
    while (1) {
        vga[0] = 'B';
        vga[1] = 0x0C;
        i++;
        if (i == 10000000) {
            i = 0;
            scheduler_tick();  /* manually switch */
        }
    }
}

void kernel_main() {
    /* enable FPU */
    __asm__ volatile (
        "mov %%cr0, %%eax\n"
        "and $0xFFFFFFFB, %%eax\n"
        "or $0x2, %%eax\n"
        "mov %%eax, %%cr0\n"
        "fninit\n"
        : : : "eax"
    );

    __asm__ volatile ("cli");
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
    print("Welcome to PrajnaOS", 0, 30, 0x09);

    gdt_init();
    idt_init();
    pit_init(100);
    ata_init();

    /* L5 — memory manager */
    pmm_init((uint32_t)&kernel_end);
    print("Memory manager ready", 12, 0, 0x02);

    void *page1 = pmm_alloc();
    void *page2 = pmm_alloc();
    pmm_free(page1);
    print("PMM initialized", 13, 0, 0x02);

    /* L6 — scheduler */
    scheduler_init();
    task_create(task1);
    task_create(task2);
    print("Scheduler ready", 14, 0, 0x02);
    __asm__ volatile ("sti");
    scheduler_start();

    wait_seconds(1);
    clear_screen();
    print("PrajnaOS>", 2, 0, 0x03);

    // __asm__ volatile ("sti");  /* enable interrupts */
    // scheduler_start();         /* jump into first task — never returns */
}