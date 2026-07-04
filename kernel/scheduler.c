#include "include/types.h"
#include "include/scheduler.h"
#include "include/pmm.h"
#include "include/ml_math.h"
#include "include/ml_infer.h"
#include "ai_kernel.h"

/* ── task table — holds all tasks ── */
 Task tasks[MAX_TASKS];

/* ── current running task index ── */
static uint32_t current_task = 0;

/* ── number of tasks created ── */
static uint32_t task_count = 0;

/* ── NEW: fallback flag — set 1 when Prajna is offline ── */
static uint8_t prajna_fallback = 0;

/* ── NEW: pending switch state ── */
static uint8_t  switch_pending = 0;
static uint32_t pending_target = 0;

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
    /* null check — abort if out of memory */
    if (stack_page == 0) return;
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
    if (task_count == 0) return;

    uint32_t i;
    uint32_t prev       = current_task;
    uint32_t best       = MAX_TASKS;
    float    best_score = -1.0f;

    /* ── NEW: run Prajna inference cycle ── */
    /* ai_kernel_tick() returns early if 50-tick interval hasn't passed */
    /* so calling it here every scheduler tick is safe and cheap        */
    ai_kernel_tick();

    /* score all tasks via ML */
    ml_score_tasks();

    /* pick best task — weigh both Prajna priority AND ml_score */
for (i = 0; i < MAX_TASKS; i++) {
    if (i == prev) continue;
    if (tasks[i].state != TASK_READY) continue;

    /* increment wait counter for every ready task not currently running */
    tasks[i].wait_ticks++;
    tasks[i].wait_time = tasks[i].wait_ticks * 10.0f;
    /* Prajna permission check */
    /* update wait_time for ML input — convert ticks to ms (100Hz = 10ms per tick) */   
    if (!prajna_fallback && ai_is_blocked(i)) continue;

    /* NEW: combined score = Prajna priority tier + raw ml_score
       priority tier is worth 0.3 per level so tier 2 = +0.6 boost
       this means Prajna's judgment weighs heavily but ml_score still matters */
    float prajna_boost = ai_get_priority(i) * 0.3f;
    float combined     = tasks[i].ml_score + prajna_boost;

    if (combined > best_score) {
        best_score = combined;
        best = i;
    }
}
 /* fallback */
    if (best == MAX_TASKS) {
        for (i = 0; i < MAX_TASKS; i++) {
            if (i == prev) continue;
            if (tasks[i].state == TASK_READY) {
                best = i;
                break;
            }
        }
    }

    if (best == MAX_TASKS) return;

    switch_pending = 1;
    pending_target = best;

}

void ml_score_tasks() {
    uint32_t i;
    float input[4];
    for (i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_DEAD) continue;
        input[0] = ml_normalize(tasks[i].cpu_usage,  0.0f, 100.0f);
        input[1] = ml_normalize(tasks[i].mem_usage,  0.0f, 100.0f);
        input[2] = ml_normalize(tasks[i].wait_time,  0.0f, 10000.0f);
        input[3] = ml_normalize(tasks[i].priority,   0.0f, 10.0f);
        tasks[i].ml_score = ml_infer(input);
    }
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
/* ── NEW: called by watchdog if Prajna hangs ── */
void scheduler_prajna_offline() {
    prajna_fallback = 1;
    ai_kernel_offline();   /* prints "Prajna offline — manual mode" */
}

void scheduler_do_pending_switch(void) {
    if (!switch_pending) return;
    switch_pending = 0;

    uint32_t prev = current_task;
    uint32_t best = pending_target;

    tasks[prev].state = TASK_READY;
    tasks[best].state = TASK_RUNNING;
    tasks[best].wait_ticks =0; //rest strvation
    current_task = best;

    context_switch(&tasks[prev].esp, tasks[best].esp);
}

