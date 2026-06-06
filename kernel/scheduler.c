#include "include/types.h"
#include "include/scheduler.h"
#include "include/pmm.h"

/* ── task table — holds all tasks ── */
static Task tasks[MAX_TASKS];

/* ── current running task index ── */
static uint32_t current_task = 0;

/* ── number of tasks created ── */
static uint32_t task_count = 0;


/* ── initialize scheduler ── */
void scheduler_init() {
    uint32_t i;

    /* mark all task slots as dead/empty */
    for (i = 0; i < MAX_TASKS; i++) {
        tasks[i].state = TASK_DEAD;
        tasks[i].stack = 0;
    }

    task_count  = 0;
    current_task = 0;
}

/* ── create a new task ── */
/* entry = function pointer — where task starts executing */
void task_create(void (*entry)()) {
    uint32_t i;

    /* find empty slot */
    for (i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_DEAD)
            break;
    }
    if (i == MAX_TASKS) return;

    /* allocate stack page */
    uint32_t stack_page = (uint32_t)pmm_alloc();
    uint32_t stack_top  = stack_page + PAGE_SIZE;

    /* set up initial stack manually
       context_switch expects: edi, esi, ebx, ebp, ret_addr
       we push these onto the new task's stack */
    uint32_t *stack = (uint32_t*)stack_top;

    *--stack = (uint32_t)entry;  /* return address = entry function */
    *--stack = 0;                /* ebp */
    *--stack = 0;                /* ebx */
    *--stack = 0;                /* esi */
    *--stack = 0;                /* edi */

    /* save ESP pointing to top of prepared stack */
    tasks[i].esp   = (uint32_t)stack;
    tasks[i].stack = stack_page;
    tasks[i].state = TASK_READY;

    task_count++;
}

/* ── scheduler tick — called by PIT every 100ms ── */
/* this is where round-robin switching happens */
void scheduler_tick() {
    uint32_t i;
    if (task_count == 0) return;

    uint32_t prev = current_task;
    uint32_t next = current_task;

    for (i = 0; i < MAX_TASKS; i++) {
        next = (next + 1) % MAX_TASKS;
        if (tasks[next].state == TASK_READY ||
            tasks[next].state == TASK_RUNNING)
            break;
    }

    if (next == prev) return;

    tasks[prev].state = TASK_READY;
    tasks[next].state = TASK_RUNNING;
    current_task = next;

    /* do the actual context switch */
    context_switch(&tasks[prev].esp, tasks[next].esp);
}
void scheduler_start() {
    /* mark first task as running */
    tasks[0].state = TASK_RUNNING;
    current_task = 0;

    /* jump into first task */
    __asm__ volatile (
        "mov %0, %%esp\n"
        "pop %%edi\n"
        "pop %%esi\n"
        "pop %%ebx\n"
        "pop %%ebp\n"
        "ret\n"
        : : "r"(tasks[0].esp)
    );
}