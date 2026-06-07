#ifndef HEAP_H
#define HEAP_H

#include "types.h"

/* ── block header — sits before every allocation ── */
typedef struct Block {
    uint32_t      size;   /* size of data area in bytes */
    uint8_t       used;   /* 1 = used, 0 = free */
    struct Block *next;   /* pointer to next block in heap */
} Block;

/* ── function declarations ── */
void  heap_init();        /* initialize heap — call once at boot */
void* kmalloc(uint32_t size);  /* allocate n bytes */
void  kfree(void *ptr);        /* free allocation */

#endif