#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

/* ── maximum number of tasks ── */
#define MAX_TASKS 8

/* ── task states ── */
#define TASK_DEAD    0   /* task slot is empty */
#define TASK_READY   1   /* task is ready to run */
#define TASK_RUNNING 2   /* task is currently running */
#define SHELL_TASK_ID  0 /* reserved task ID for shell */

/* ── task structure ── */
typedef struct {
    uint32_t esp;      /* saved stack pointer — updated on context switch */
    uint32_t stack;    /* physical address of stack page from pmm_alloc */
    uint8_t  state;    /* TASK_DEAD, TASK_READY, TASK_RUNNING */
     /* ── ML fields — added for L8 ── */
    float cpu_usage;    /* 0.0 to 100.0 */
    float mem_usage;    /* 0.0 to 100.0 */
    float wait_time;    /* 0.0 to 10000.0 ms */
    float priority;     /* 0.0 to 10.0 */
    float ml_score;     /* assigned by ml_infer() — higher = run first */
    uint32_t wait_ticks; /*NEW -- increments every tick while task is READY but not running*/
} Task;

/* ── function declarations ── */
void scheduler_init();
void task_create(void (*entry)());
void scheduler_tick();
void context_switch(uint32_t *old_esp, uint32_t new_esp);
void scheduler_start();

/* ml scheduler functions */
void ml_score_tasks();
void scheduler_prajna_offline(void);
void scheduler_do_pending_switch(void);

extern Task tasks[MAX_TASKS];
#endif