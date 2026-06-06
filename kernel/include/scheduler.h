#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

/* ── maximum number of tasks ── */
#define MAX_TASKS 8

/* ── task states ── */
#define TASK_DEAD    0   /* task slot is empty */
#define TASK_READY   1   /* task is ready to run */
#define TASK_RUNNING 2   /* task is currently running */

/* ── task structure ── */
typedef struct {
    uint32_t esp;      /* saved stack pointer — updated on context switch */
    uint32_t stack;    /* physical address of stack page from pmm_alloc */
    uint8_t  state;    /* TASK_DEAD, TASK_READY, TASK_RUNNING */
} Task;

/* ── function declarations ── */
void scheduler_init();
void task_create(void (*entry)());
void scheduler_tick();
void context_switch(uint32_t *old_esp, uint32_t new_esp);
void scheduler_start();

#endif