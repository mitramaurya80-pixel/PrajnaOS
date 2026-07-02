#include "ai_kernel.h"
#include "pit.h"
#include "pmm.h"
#include "scheduler.h"
#include "types.h"
#include "ml_math.h"

#define AI_TICK_INTERVAL  50
#define PMM_ALERT_THRESHOLD  64   /* pages — tune this later */
#define STARVATION_THRESHOLD 500 /*ticks -- 5  seconds at 100Hz*/
#define HISTORY_LEN 8 /*how many past samples to remember per task*/
static uint32_t last_ai_tick = 0;
static uint8_t  ai_online    = 1;
static sys_state_t current_state = STATE_NORMAL;

// print function for print alert messages to VGA
static void print(const char *msg, int row, int col, uint8_t color) {
    char *vga = (char *)0xB8000;
    int col_pos = col;
    for (int i = 0; msg[i] != '\0'; i++) {
        if (msg[i] == '\n') { row++; col_pos = 0; continue; }
        if (msg[i] == '\t') { col_pos += 4; continue; }
        int index = (row * 80 + col_pos) * 2; 
        vga[index]     = msg[i];
        vga[index + 1] = color;
        col_pos++;
    }
}/* ── anomaly helpers — must be above ai_sense() ── */
static float avg_history(const float *hist, uint8_t filled, uint8_t len) {
    uint8_t count = filled ? len : len;
    float sum = 0.0f;
    for (uint8_t i = 0; i < count; i++) sum += hist[i];
    return sum / (float)count;
}

static uint8_t is_anomalous(float current, float avg) {
    if (avg < 1.0f) return 0;
    float diff = current - avg;
    if (diff < 0) diff = -diff;
    float ratio = diff / avg;
    return (ratio > 0.6f) ? 1 : 0;
}

/*History*/
typedef struct
{
    float cpu_history[HISTORY_LEN];
    float mem_history[HISTORY_LEN];
    uint8_t history_index;   /* circular buffer position */
    uint8_t history_filled;  /* 0 until buffer wraps once */
    /* data */
}ai_history_t;

static ai_history_t task_history[MAX_TASKS];


static ai_permission_t perm_table[MAX_TASKS];

/* ── Sense ── */
typedef struct {
    uint32_t ticks;
    uint32_t free_pages;
    uint32_t ready_count;
    uint32_t dead_count;
    float    sensor_a;   /* hardcoded for now */
    float    sensor_b;
    float    ml_scores[MAX_TASKS];  /* optional — for debugging */  
    uint8_t anomaly[MAX_TASKS];
} ai_input_t;

static void ai_sense(ai_input_t *in) {
    in->ticks      = pit_get_ticks();
    in->free_pages = pmm_free_pages();
    in->sensor_a   = 1.4f;
    in->sensor_b   = 0.2f;

    in->ready_count = 0;
    in->dead_count  = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_READY)   in->ready_count++;
        if (tasks[i].state == TASK_DEAD)    in->dead_count++;

        /* Optional: calculate ML score for each task */
        in->ml_scores[i] = tasks[i].ml_score;
        in->anomaly[i]=0; /*default  not anomaly*/
        if (tasks[i].state==TASK_DEAD) continue;
        /* NEW: anomaly check using task history */
        ai_history_t *h = &task_history[i];

        if (h->history_filled) {
            float cpu_avg = avg_history(h->cpu_history, h->history_filled, HISTORY_LEN);
            float mem_avg = avg_history(h->mem_history, h->history_filled, HISTORY_LEN);

            uint8_t cpu_anom = is_anomalous(tasks[i].cpu_usage, cpu_avg);
            uint8_t mem_anom = is_anomalous(tasks[i].mem_usage, mem_avg);

            in->anomaly[i] = (cpu_anom || mem_anom) ? 1 : 0;
        }

        /* record current values into circular buffer */
        h->cpu_history[h->history_index] = tasks[i].cpu_usage;
        h->mem_history[h->history_index] = tasks[i].mem_usage;
        h->history_index = (h->history_index + 1) % HISTORY_LEN;

        if (h->history_index == 0) h->history_filled = 1;
    }
}



typedef struct {
    sys_state_t sys_state;
    uint8_t     perm[MAX_TASKS];      /* allowed/blocked per task */
    uint8_t     priority[MAX_TASKS];  /* 0/1/2 per task           */
} ai_decision_t;

static void ai_think(const ai_input_t *in, ai_decision_t *out) {


    /* ── system state ── */
    if (in->free_pages < PMM_ALERT_THRESHOLD)
        out->sys_state = STATE_ALERT;
    else if (in->ready_count > 4)
        out->sys_state = STATE_NORMAL;
    else
        out->sys_state = STATE_CALM;

    /* ── per-task decisions ── */
    for (int i = 0; i < MAX_TASKS; i++) {

        /* dead tasks get nothing */
        if (tasks[i].state == TASK_DEAD) {
            out->perm[i]     = 0;
            out->priority[i] = 0;
            continue;
        }

        /* ALERT — only shell runs */
        if (out->sys_state == STATE_ALERT && i != SHELL_TASK_ID) {
            out->perm[i]     = 0;
            out->priority[i] = 0;
            continue;
        }

        /* task is allowed — set priority from ml_score */
        out->perm[i] = 1;

        if (in->ml_scores[i] > 0.7f)
            out->priority[i] = 2;   /* HIGH   */
        else if (in->ml_scores[i] > 0.4f)
            out->priority[i] = 1;   /* NORMAL */
        else
            out->priority[i] = 0;   /* LOW    */
        /* NEW: anomaly guard — suspicious task gets capped at NORMAL,
           never HIGH, until it stabilizes */
        if (in->anomaly[i] && out->priority[i] > 1) {
            out->priority[i] = 1;
        }
        /* NEW: starvation guard — task waited too long, force it to run */
        if (tasks[i].wait_ticks > STARVATION_THRESHOLD) {
            out->priority[i] = 2;   /* force HIGH */
            out->perm[i]     = 1;   /* force allowed even in ALERT */
        }
        
    }

    /* ── shell always gets at least NORMAL priority ── */
    if (tasks[SHELL_TASK_ID].state != TASK_DEAD) {
        out->perm[SHELL_TASK_ID] = 1;
        if (out->priority[SHELL_TASK_ID] < 1)
            out->priority[SHELL_TASK_ID] = 1;
    }

    /* ── save state ── */
    current_state = out->sys_state;
}
 /* TEMP DEBUG — remove after verifying */
    void print_num(int row, int col, uint8_t color, uint8_t n) {
    char *vga = (char *)0xB8000;
    int idx = (row * 80 + col) * 2;
    vga[idx]     = '0' + (n % 10);
    vga[idx + 1] = color;
}

/* ── Act ── */
static void ai_act(const ai_decision_t *dec) {
        /* in ai_act(), always print current state */
    if (dec->sys_state == STATE_CALM)
        print("[PRAJNA] CALM", 22, 0, 0x03);
    else if (dec->sys_state == STATE_NORMAL)
        print("[PRAJNA] NORMAL", 22, 0, 0x03);
    else
        print("[PRAJNA] ALERT", 22, 0, 0x04);
    for (int i = 0; i < MAX_TASKS; i++) {
        perm_table[i].allowed  = dec->perm[i];
        perm_table[i].priority = dec->priority[i];
    }

    /* Optional: print system state to VGA */
    if (dec->sys_state == STATE_ALERT)
        print("[PRAJNA] ALERT -- low memory", 14, 0, 0x03);

/* in ai_act(), after writing perm_table: */
for (int i = 0; i < MAX_TASKS; i++) {
    print_num(21, i * 2, 0x0E, perm_table[i].priority);
}
}
/* NEW: return Prajna's priority for a task — 0/1/2 */
uint8_t ai_get_priority(uint32_t task_id) {
    if (task_id >= MAX_TASKS) return 1;   /* default normal */
    return perm_table[task_id].priority;
}

/* ── Main loop hook ── */
/* Call this from your kernel main loop or timer ISR */
void ai_kernel_tick(void) {
    if (!ai_online) return;

    uint32_t now = pit_get_ticks();
    if (now - last_ai_tick < AI_TICK_INTERVAL) return;  
    last_ai_tick = now;

    ai_input_t    in;
    ai_decision_t dec;

    ai_sense(&in);
    ai_think(&in, &dec);
    ai_act(&dec);
}

/* Fallback — call if ai_kernel_tick watchdog times out */
void ai_kernel_offline(void) {
    ai_online = 0;
    print("[PRAJNA] offline -- manual mode", 14, 0, 0x04);  /* 0x04 = red, Prajna is down */
    /* scheduler falls back to round-robin automatically */
}
/* in ai_kernel.c */
uint8_t ai_is_blocked(uint32_t task_id) {
    if (task_id >= MAX_TASKS) return 0;
    return (perm_table[task_id].allowed == 0) ? 1 : 0;
}
sys_state_t ai_get_state(void) {
    return current_state;   /* static variable you already track in ai_think() */
}
