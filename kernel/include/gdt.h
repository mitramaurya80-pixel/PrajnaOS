#ifndef GDT_H       /* if GDT_H is not defined yet...     */
#define GDT_H       /* define it (stops double-including) */

#include "types.h"  /* our own uint8_t, uint16_t, uint32_t */

/*
 * ONE GDT ENTRY = 8 bytes exactly
 * Describes one memory segment to the CPU
 * Think of it like one row in a table of memory rules
 */
struct gdt_entry {
    uint16_t limit_low;    /* bits 0-15  of segment size             */
    uint16_t base_low;     /* bits 0-15  of segment start address    */
    uint8_t  base_mid;     /* bits 16-23 of segment start address    */
    uint8_t  access;       /* who can use this and what they can do  */
    uint8_t  granularity;  /* flags + bits 16-19 of segment size     */
    uint8_t  base_high;    /* bits 24-31 of segment start address    */
} __attribute__((packed));
/* __attribute__((packed)) = tell compiler:
   do NOT add padding bytes between fields!
   Without this the struct would be wrong size
   and CPU would read garbage */

/*
 * GDT POINTER = 6 bytes
 * This is what we give to the CPU with lgdt instruction
 * It tells CPU: "GDT is THIS big and located HERE"
 */
struct gdt_ptr {
    uint16_t limit;  /* size of entire GDT in bytes minus 1        */
    uint32_t base;   /* memory address where GDT table starts      */
} __attribute__((packed));

/* function declaration - defined in gdt.c */
void gdt_init();

#endif  /* end of the #ifndef GDT_H guard */
