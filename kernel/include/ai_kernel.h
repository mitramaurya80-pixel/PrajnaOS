#ifndef AI_KERNEL_H
#define AI_KERNEL_H

#include "types.h"
#include "scheduler.h"

/* ── permission entry for one task ── */
typedef struct {
    uint8_t allowed;    /* 0 = blocked, 1 = allowed */
    uint8_t priority;   /* 0=low, 1=normal, 2=high  */
} ai_permission_t;

/* ── system state visible to other modules if needed ── */
typedef enum {
    STATE_CALM,
    STATE_NORMAL,
    STATE_ALERT
} sys_state_t;
uint8_t ai_get_priority(uint32_t task_id);
typedef struct {
    uint32_t    tick;
    sys_state_t state;
    uint8_t     perm[MAX_TASKS];
    uint8_t     priority[MAX_TASKS];
    uint8_t     anomaly[MAX_TASKS];
} prajna_event_t;
uint8_t ai_get_log(prajna_event_t *out, uint8_t count);
/* ── public API ── */
uint8_t     ai_is_blocked(uint32_t task_id);
void        ai_kernel_tick(void);
void        ai_kernel_offline(void);
sys_state_t ai_get_state(void);   /* optional — useful for shell/debug output */

#endif