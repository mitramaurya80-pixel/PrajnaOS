#ifndef PMM_H
#define PMM_H

#include "types.h"

/* ── page size — 4KB = 4096 bytes ── */
#define PAGE_SIZE    4096

/* ── total memory we manage — 512MB ── */
#define MEM_SIZE     (512 * 1024 * 1024)

/* ── total number of pages ── */
/* 512MB / 4KB = 131072 pages */
#define TOTAL_PAGES  (MEM_SIZE / PAGE_SIZE)

/* ── function declarations ── */

/* initialize memory manager
   kernel_end_addr = where kernel ends in memory
   we mark everything below this as used */
void pmm_init(uint32_t kernel_end_addr);

/* allocate one 4KB page
   returns physical address of page
   returns 0 if out of memory */
void* pmm_alloc();

/* free one page
   addr = address returned by pmm_alloc() */
void pmm_free(void *addr);

/* count how many pages are still free */
uint32_t pmm_free_pages();

#endif