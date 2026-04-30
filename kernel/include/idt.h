#ifndef IDT_H
#define IDT_H

#include "types.h"

/*
 * ONE IDT ENTRY = 8 bytes exactly
 * Tells CPU: "when interrupt N fires, jump to THIS function"
 * Think of it like one row in a phonebook of emergency handlers
 */
struct idt_entry {
    uint16_t base_low;   /* bits 0-15  of handler function address  */
    uint16_t selector;   /* which GDT segment to use = always 0x08  */
    uint8_t  zero;       /* always 0, reserved by Intel CPU design   */
    uint8_t  flags;      /* type of gate = 0x8E for kernel interrupt */
    uint16_t base_high;  /* bits 16-31 of handler function address  */
} __attribute__((packed));

/*
 * IDT POINTER = 6 bytes
 * Same idea as gdt_ptr — tells CPU where IDT table is
 */
struct idt_ptr {
    uint16_t limit;  /* size of entire IDT in bytes minus 1         */
    uint32_t base;   /* memory address where IDT table starts       */
} __attribute__((packed));


/* function declaration - defined in idt.c */
void idt_init();

#endif
