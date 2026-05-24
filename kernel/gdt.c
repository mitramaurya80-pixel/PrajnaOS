#include "include/gdt.h"  /* our gdt_entry and gdt_ptr structs */

/* our GDT table — 3 entries:
   [0] = null entry (CPU rule — must always be empty)
   [1] = kernel code segment
   [2] = kernel data segment */
struct gdt_entry gdt[3];

/* the pointer we give to CPU via lgdt instruction */
struct gdt_ptr gp;

/* this function lives in gdt.asm
   we declare it here so C knows it exists */
extern void gdt_flush(uint32_t);

/*
 * gdt_set() — fills one entry in the GDT table
 *
 * i      = which entry to fill (0, 1, or 2)
 * base   = where does this segment start in memory
 * limit  = how big is this segment
 * access = who can use it and how (0x9A or 0x92)
 * gran   = granularity flags (0xCF for 32-bit 4GB)
 */
static void gdt_set(int i, uint32_t base, uint32_t limit,
                    uint8_t access, uint8_t gran) {

    /* split the base address into 3 pieces
       because Intel designed the struct this weird way */
      gdt[i].base_low  = (base & 0xFFFF);         /* grab bits 0-15  */
      gdt[i].base_mid  = (base >> 16) & 0xFF;     /* grab bits 16-23 */
      gdt[i].base_high = (base >> 24) & 0xFF;     /* grab bits 24-31 */

      /* split the limit into 2 pieces */
      gdt[i].limit_low   = (limit & 0xFFFF);      /* grab bits 0-15  */

      /* upper 4 bits of limit go into top of granularity byte
         lower 4 bits of gran = flags (page size, 32-bit mode) */
      gdt[i].granularity = ((limit >> 16) & 0x0F) /* bits 16-19 of limit */
                        | (gran & 0xF0);          /* top 4 bits of flags */

      /* access byte goes in directly */
      gdt[i].access = access;
}

/*
 * gdt_init() — sets up the GDT and loads it into the CPU
 * called once from kernel_main()
 */
void gdt_init() {

    /* tell gp how big the GDT is and where it lives */
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    /* sizeof(gdt_entry) = 8 bytes, 3 entries = 24 bytes, minus 1 = 23 */

    gp.base = (uint32_t)&gdt;
    /* &gdt = address of our gdt array in memory
       cast to uint32_t because pointer is 32-bit */

    /* entry 0 = null — all zeros, CPU rule */
    gdt_set(0, 0, 0, 0, 0);

    /* entry 1 = kernel code segment
       base=0, limit=4GB, access=0x9A, gran=0xCF
       0x9A = 1001 1010 = present, ring0, code, readable
       0xCF = 1100 1111 = 32-bit protected mode, 4KB pages */
    gdt_set(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* entry 2 = kernel data segment
       base=0, limit=4GB, access=0x92, gran=0xCF
       0x92 = 1001 0010 = present, ring0, data, writable */
    gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* call assembly function to actually load GDT into CPU
       we pass the address of gp (our gdt_ptr struct) */
    gdt_flush((uint32_t)&gp);
}
