    #include "ai_kernel.h"
    #include "pit.h"
    #include "pmm.h"
    #include "scheduler.h"
    #include "types.h"
    #include "ml_math.h"
    #include "klib.h"

    #define AI_TICK_INTERVAL  50
    #define PMM_ALERT_THRESHOLD  64   /* pages — tune this later */
    #define STARVATION_THRESHOLD 5 /*5 sec*/
    #define EVENT_LOG_SIZE 20
    static uint32_t last_ai_tick = 0;
    static uint8_t  ai_online    = 1;
    static sys_state_t current_state = STATE_CALM;
    static uint8_t starvation_warned = 0;   /* print warning only once */

    /* vga print */
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
    }
    /* ── anomaly helpers — must be above ai_sense() ── */
    static float avg_history(const float *hist, uint8_t filled, uint8_t len, uint8_t index) {
        uint8_t count = filled ? len : index;
        if(count == 0) return 0.0f;
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

    static prajna_event_t event_log[EVENT_LOG_SIZE];
    static uint8_t        event_index  = 0;   /* next write position */
    static uint8_t        event_filled = 0;   /* 1 once buffer wraps */
    // history of free pages and ticks for shell command
    typedef struct
    {
        uint32_t free_pages[HISTORY_LEN];
        uint8_t history_index;   /* circular buffer position */
        uint8_t history_filled;  /* 0 until buffer wraps once */
        uint32_t ticks[HISTORY_LEN];
    }mem_history_t;
    // trend of free pages over time
    typedef struct {
        uint32_t oldest_pages;
        uint32_t newest_pages;
        uint32_t oldest_tick;
        uint32_t newest_tick;
    } mem_trend_t;
    static mem_trend_t mem_trend;
    static mem_history_t mem_history;

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
    /*memory prediction */
    static void ai_predict_memory(void)
    {
        if (!mem_history.history_filled) {
            return;
        }

        if (mem_trend.newest_pages >= mem_trend.oldest_pages) {
            return;
        }

        uint32_t pages_lost    = mem_trend.oldest_pages - mem_trend.newest_pages;
        uint32_t ticks_elapsed = mem_trend.newest_tick - mem_trend.oldest_tick;

        if (ticks_elapsed == 0) {
            return;
        }

        if (mem_trend.newest_pages <= PMM_ALERT_THRESHOLD) {
            return;
        }

        uint32_t remaining= mem_trend.newest_pages - PMM_ALERT_THRESHOLD;
        uint32_t ticks_to_alert = (remaining * ticks_elapsed) / pages_lost;

        if (ticks_to_alert <= 4000) {
            print("[PRAJNA] Warning: memory depleting fast, act fast", 22, 0, 0x0E);
        } else {
            print("[PRAJNA] Debug: trend ok, not urgent", 22, 0, 0x0A);

        }
    }
    /* NEW: update memory history buffer + trend — called once per sense(), not per task */
    static void update_mem_trend(uint32_t free_pages, uint32_t ticks)
    {
        mem_history.free_pages[mem_history.history_index] = free_pages;
        mem_history.ticks[mem_history.history_index]      = ticks;
        mem_history.history_index = (mem_history.history_index + 1) % HISTORY_LEN;
        if (mem_history.history_index == 0) mem_history.history_filled = 1;

        mem_trend.newest_pages = free_pages;
        mem_trend.newest_tick  = ticks;
        mem_trend.oldest_pages = mem_history.free_pages[mem_history.history_index];
        mem_trend.oldest_tick  = mem_history.ticks[mem_history.history_index];

        ai_predict_memory();  /* check for memory depletion trend and alert if needed */
    }
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
            if(tasks[i].state != TASK_DEAD){
                tasks[i].cpu_usage = (float)tasks[i].run_ticks / (float)AI_TICK_INTERVAL * 100.0f;
                tasks[i].run_ticks = 0;
            }
            /* Optional: calculate ML score for each task */
            in->ml_scores[i] = tasks[i].ml_score;
            in->anomaly[i]=0; /*default  not anomaly*/
            if (tasks[i].state==TASK_DEAD) continue;
            /* NEW: anomaly check using task history */
            ai_history_t *h = &task_history[i];

            if (h->history_filled) {
                float cpu_avg = avg_history(h->cpu_history, h->history_filled,h->history_index,HISTORY_LEN);
                float mem_avg = avg_history(h->mem_history, h->history_filled, h->history_index, HISTORY_LEN);

                uint8_t cpu_anom = is_anomalous(tasks[i].cpu_usage, cpu_avg);
                uint8_t mem_anom = is_anomalous(tasks[i].mem_usage, mem_avg);

                in->anomaly[i] = (cpu_anom || mem_anom) ? 1 : 0;
            /* check for memory depletion trend and alert if needed */
            }

            /* record current values into circular buffer */
            h->cpu_history[h->history_index] = tasks[i].cpu_usage;
            h->mem_history[h->history_index] = tasks[i].mem_usage;
            h->history_index = (h->history_index + 1) % HISTORY_LEN;

            if (h->history_index == 0) h->history_filled = 1;
        }

        /* ── memory trend — once per sense(), not per task ── */
        update_mem_trend(in->free_pages, in->ticks);


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


    /* ── Act ── */
    /* ── Act ── */
    static void ai_act(const ai_decision_t *dec) {

        /* ── log decision to ring buffer ── */
        prajna_event_t *ev = &event_log[event_index];
        ev->tick  = pit_get_ticks();
        ev->state = dec->sys_state;
        ev->free_pages_at_decision = pmm_free_pages();
        ev->starvation_task_id = 0xFF;  /* default none */
        ev->anomaly_task_id = 0xFF;     /* default none */
        for (int i = 0; i < MAX_TASKS; i++) {
            ev->perm[i]     = dec->perm[i];
            ev->priority[i] = dec->priority[i];
            if(tasks[i].wait_ticks > STARVATION_THRESHOLD && ev->starvation_task_id == 0xFF) {
                ev->starvation_task_id = i;
            }
        }
        event_index = (event_index + 1) % EVENT_LOG_SIZE;
        if (event_index == 0) event_filled = 1;


        /* ── write permission table ── */
        for (int i = 0; i < MAX_TASKS; i++) {
            perm_table[i].allowed  = dec->perm[i];
            perm_table[i].priority = dec->priority[i];
        }

        /* ── NEW: proactive alerts on row 23 ── */
        /* clear alert row first */
        print("                                                  ", 23, 0, 0x07);

        /* alert 1 — low memory warning before ALERT threshold */
        uint32_t free_pages = pmm_free_pages();
        if (free_pages < PMM_ALERT_THRESHOLD * 2) {
            print("[PRAJNA] Warning: memory low, act fast", 23, 0, 0x0E);
        }

        /* alert 2 — starvation warning — print only once */
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state == TASK_DEAD) continue;
            if (tasks[i].wait_ticks > STARVATION_THRESHOLD) {
                if (!starvation_warned) {
                    print("[PRAJNA] Warning: task starvation detected", 23, 0, 0x0E);
                    starvation_warned = 1;
                }
                break;
            }
        }

        /* reset warning when no starvation detected */
        if (starvation_warned) {
            uint8_t any = 0;
            for (int i = 0; i < MAX_TASKS; i++) {
                if (tasks[i].state == TASK_DEAD) continue;
                if (tasks[i].wait_ticks > STARVATION_THRESHOLD) { any = 1; break; }
            }
            if (!any) starvation_warned = 0;
        }

        /* alert 3 — anomaly warning */
        for (int i = 0; i < MAX_TASKS; i++) {
            if (tasks[i].state == TASK_DEAD) continue;
            if (dec->perm[i] == 1 && dec->priority[i] < 2) {
                /* check if anomaly caused priority cap */
                /* we detect this indirectly — ml_score > 0.7 but priority is 1 */
                if (perm_table[i].priority == 1 && tasks[i].ml_score > 0.7f) {
                    print("[PRAJNA] Warning: anomaly detected in task", 23, 0, 0x0C);
                    break;
                }
            }
        }
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
    /* NEW: fill a buffer with last N log entries for shell display */
    uint8_t ai_get_log(prajna_event_t *out, uint8_t count) {
        uint8_t total = event_filled ? EVENT_LOG_SIZE : event_index;
        if (count > total) count = total;

        /* read backwards from most recent */
        for (uint8_t i = 0; i < count; i++) {
            uint8_t idx = (event_index - 1 - i + EVENT_LOG_SIZE) % EVENT_LOG_SIZE;
            out[i] = event_log[idx];
        }
        return count;   /* actual entries returned */
    }
    /* return Prajna's priority for a task — 0/1/2 */
    uint8_t ai_get_priority(uint32_t task_id) {
        if (task_id >= MAX_TASKS) return 1;   /* default normal */
        return perm_table[task_id].priority;
    }
    uint8_t ai_get_memory_history(uint32_t pages[], uint32_t ticks[])
    {
        uint8_t count = mem_history.history_filled ?
                        HISTORY_LEN :
                        mem_history.history_index;

        if (count == 0)
            return 0;

        for (uint8_t i = 0; i < count; i++) {
            uint8_t idx;

            if (mem_history.history_filled)
                idx = (mem_history.history_index + i) % HISTORY_LEN;
            else
                idx = i;

            pages[i] = mem_history.free_pages[idx];
            ticks[i] = mem_history.ticks[idx];
        }

        return count;
    }