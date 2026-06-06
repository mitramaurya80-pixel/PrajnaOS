#include "include/types.h"
#include "include/pmm.h"

/* ── bitmap — one bit per page ──
   bit = 0 means page is FREE
   bit = 1 means page is USED
   TOTAL_PAGES / 8 = number of bytes needed
   131072 pages / 8 = 16384 bytes = 16KB for bitmap */
static uint8_t bitmap[TOTAL_PAGES / 8];

/* ── mark one page as USED ── */
static void pmm_set_used(uint32_t page) {
    /* page / 8 → which byte in bitmap */
    /* page % 8 → which bit inside that byte */
    /* OR with 1 shifted left → set that bit to 1 */
    bitmap[page / 8] |= (1 << (page % 8));
}

/* ── mark one page as FREE ── */
static void pmm_set_free(uint32_t page) {
    /* AND with inverted mask → set that bit to 0 */
    bitmap[page / 8] &= ~(1 << (page % 8));
}

/* ── check if a page is free ── */
static uint8_t pmm_is_free(uint32_t page) {
    /* if bit is 0 → free → return 1 (true) */
    return !(bitmap[page / 8] & (1 << (page % 8)));
}

/* ── initialize memory manager ── */
void pmm_init(uint32_t kernel_end_addr) {
    uint32_t i;

    /* step 1 — clear entire bitmap — all pages free */
    for (i = 0; i < TOTAL_PAGES / 8; i++)
        bitmap[i] = 0;

    /* step 2 — mark first 1MB as used
       BIOS, VGA buffer, interrupt tables live here
       0x00000 to 0xFFFFF = first 256 pages */
    for (i = 0; i < 256; i++)
        pmm_set_used(i);

    /* step 3 — mark kernel pages as used
       kernel lives from 1MB to kernel_end_addr
       calculate how many pages kernel takes */
    uint32_t kernel_pages = (kernel_end_addr / PAGE_SIZE) + 1;
    for (i = 0; i < kernel_pages; i++)
        pmm_set_used(i);
}

/* ── allocate one free page ── */
void* pmm_alloc() {
    uint32_t i, bit;

    /* scan bitmap byte by byte */
    for (i = 0; i < TOTAL_PAGES / 8; i++) {

        /* if all 8 bits are 1 → all 8 pages used → skip this byte */
        if (bitmap[i] == 0xFF)
            continue;

        /* found a byte with at least one free page */
        /* scan each bit in this byte */
        for (bit = 0; bit < 8; bit++) {

            /* check if this bit is 0 (free) */
            if (pmm_is_free(i * 8 + bit)) {

                /* mark it as used */
                pmm_set_used(i * 8 + bit);

                /* return physical address of this page */
                /* page number × page size = physical address */
                return (void*)((i * 8 + bit) * PAGE_SIZE);
            }
        }
    }

    /* no free pages found */
    return 0;
}

/* ── free one page ── */
void pmm_free(void *addr) {
    /* convert address back to page number */
    uint32_t page = (uint32_t)addr / PAGE_SIZE;

    /* mark it as free */
    pmm_set_free(page);
}

/* ── count free pages ── */
uint32_t pmm_free_pages() {
    uint32_t i, bit, count = 0;

    /* scan every bit in bitmap */
    for (i = 0; i < TOTAL_PAGES / 8; i++)
        for (bit = 0; bit < 8; bit++)
            if (pmm_is_free(i * 8 + bit))
                count++;

    return count;
}