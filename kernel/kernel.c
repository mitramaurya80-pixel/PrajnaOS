#include "include/pmm.h"
#include "include/gdt.h"
#include "include/idt.h"
#include "include/pit.h"
#include "include/ata.h"
#include "include/fat32.h"
#include "include/ml_math.h"
#include "include/shell.h"
#include "include/scheduler.h"
#include "include/heap.h"
#include "include/ml_weights.h"
#include "include/ml_infer.h"
#include "include/ai_kernel.h"
#include "include/klib.h"

extern void clear_screen(void);
extern uint32_t kernel_end;
uint8_t init_status[5]={0}; /* height of the status bar in rows */
enum {
    INIT_FAT32,
    INIT_HEAP,
    INIT_SCHEDULER,
    INIT_ML
};

static void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* direct VGA print — used before shell is running */
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

/* boot greeting — Prajna speaks before shell appears */
static void boot_time_greeting(void) {

    /* clear screen first */
    clear_screen();
    print(".....PrajnaOS.....", 0, 31, 0x0A);


    /* row 3 — FAT32 */
    if (init_status[INIT_FAT32] == 1)
        print("[PRAJNA] FAT32     : FAILED", 3, 0, 0x0C);
    else  
        print("[PRAJNA] FAT32     : mounted", 3, 0, 0x0A);

    /* row 4 — ML model */
    if (init_status[INIT_ML] == 1){
        print("[PRAJNA] ML model  : FAILED",  4, 0, 0x0C);ai_kernel_offline();} 
    else{print("[PRAJNA] ML model  : loaded",  4, 0, 0x0A);}

    if(init_status[INIT_HEAP] == 1) print("[PRAJNA] Heap      : FAILED", 4, 40, 0x0C);

    /* row 5 — state */
    sys_state_t state = ai_get_state();
    if (state == STATE_CALM)
        print("[PRAJNA] State     : CALM ", 5, 0, 0x0A);
    else if (state == STATE_NORMAL)
        print("[PRAJNA] State     : NORMAL ",      5, 0, 0x0E);
    else
        print("[PRAJNA] State     : ALERT  ",       5, 0, 0x0C);

    
    wait(5);  /* wait for 5 ticks (50ms) before clearing screen */

    clear_screen();  /* clear again before shell appears */
    /* row 7 — blank before shell */
}
/* shell runs as a real kernel task */
void shell_task() {
    while (1) {
        /* shell input comes via keyboard interrupt */
        /* this task just keeps the system alive */
        __asm__ volatile ("hlt");  /* sleep until next interrupt */
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

    /* hide cursor */
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);

    /* ── hardware init ── */
    gdt_init();
    idt_init();
    pit_init(100);
    ata_init();

    /* ── memory ── */
    pmm_init((uint32_t)&kernel_end);

    /* ── FAT32 ── */
    if(fat32_init(2048)) init_status[INIT_FAT32] = 1;  /* FAT32 init failed */

    /* ── heap ── */
    if(heap_init()) init_status[INIT_HEAP] = 1;  /* heap init failed */

    /* ── scheduler ── */
    scheduler_init();

    /* ── ML weights ── */
    if(ml_weights_load()) init_status[INIT_ML] = 1;
    /* tasks creating*/
    task_create(shell_task);  /* create shell task */
    /* ── Prajna boot greeting ── */
    boot_time_greeting();
    draw_top_bar();

    /* ── start ── */
    __asm__ volatile ("sti");
    scheduler_start();   /* never returns */
}

