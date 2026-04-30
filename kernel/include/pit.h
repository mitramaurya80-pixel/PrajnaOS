#ifndef PIT_H
#define PIT_H

#include "types.h"

void pit_init(uint32_t hz);        /* set timer frequency */
uint32_t pit_get_ticks(void);      /* get current tick count */
void pit_handler(void);  /* called every timer interrupt from idt.c */

#endif