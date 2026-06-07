#include "include/types.h"
#include "include/heap.h"
#include "include/pmm.h"

/* ── heap start — pointer to first block ── */
static Block *heap_start = 0;

/* ── initialize heap ── */
void heap_init() {
    /* get one 4KB page from pmm */
    heap_start = (Block*)pmm_alloc();

    /* set up first block — one big free block */
    heap_start->size = PAGE_SIZE - sizeof(Block);  /* full page minus header */
    heap_start->used = 0;                           /* free */
    heap_start->next = 0;                           /* no next block */
}

/* ── allocate n bytes ── */
void* kmalloc(uint32_t size) {
    Block *current = heap_start;

    /* scan linked list for a free block big enough */
    while (current) {

        /* found a free block that fits */
        if (!current->used && current->size >= size) {

            /* split block if leftover space is big enough */
            /* leftover = current->size - size */
            /* only split if leftover > sizeof(Block) + 4 bytes */
            if (current->size > size + sizeof(Block) + 4) {

                /* create new free block after this allocation */
                Block *new_block = (Block*)((uint8_t*)current
                                   + sizeof(Block) + size);
                new_block->size = current->size - size - sizeof(Block);
                new_block->used = 0;
                new_block->next = current->next;

                /* update current block */
                current->size = size;
                current->next = new_block;
            }

            /* mark as used */
            current->used = 1;

            /* return pointer to data area — just after header */
            return (void*)((uint8_t*)current + sizeof(Block));
        }

        current = current->next;
    }

    /* no free block found — out of heap memory */
    return 0;
}

/* ── free an allocation ── */
void kfree(void *ptr) {
    if (!ptr) return;  /* ignore null pointer */

    /* get header — it's sizeof(Block) bytes before ptr */
    Block *block = (Block*)((uint8_t*)ptr - sizeof(Block));

    /* mark as free */
    block->used = 0;

    /* merge with next block if it's also free — prevents fragmentation */
    if (block->next && !block->next->used) {
        block->size += sizeof(Block) + block->next->size;
        block->next  = block->next->next;
    }
}