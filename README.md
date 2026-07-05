# PrajnaOS

> *Consciousness. Intelligence. Control.*

PrajnaOS is a bare metal x86 operating system built from scratch in C and x86 Assembly
by a 2nd year CSE + Data Science student at a tier 3 college in Jaipur, India.

Named after the Sanskrit word for highest intelligence, PrajnaOS is designed with
AI as its primary kernel controller — not as an app running on top of an OS,
but as the core decision-maker running directly in kernel space with no Linux,
no stdlib, no external dependencies of any kind.

---

## What makes PrajnaOS different

Every existing OS uses a fixed scheduling algorithm — round-robin, CFS, priority queues.
PrajnaOS replaces the scheduler's brain with a trained ML model and an AI kernel called
Prajna that runs a Sense → Think → Act loop every 50ms.

Prajna reads system state, classifies it using neural network inference loaded from disk,
decides which tasks are allowed to run and at what priority, and writes those decisions
into a permission table that the scheduler reads before every context switch.

**AI is not an app on top. AI is the kernel.**

---

## Architecture

```
Hardware (CPU, RAM, ATA disk, PS/2 keyboard, VGA)
          │
Physical Memory Manager (bitmap allocator)
          │
FAT32 Filesystem (read, write, create, directory traversal)
          │
ML Inference Engine (neural network, weights loaded from IRIS.BIN)
          │
┌─────────────────────────────────────────────┐
│              PRAJNA AI KERNEL               │
│                                             │
│  Sense → Think → Act  (every 50 ticks)     │
│                                             │
│  Inputs:  tick count, free pages,           │
│           task states, ml_scores,           │
│           anomaly history                  │
│                                             │
│  Outputs: task permission (allow/block)     │
│           task priority (0/1/2)             │
│           system state (CALM/NORMAL/ALERT)  │
│           proactive alerts                  │
│           event log (ring buffer)           │
└──────────────────┬──────────────────────────┘
                   │
          AI-aware Scheduler
          (ML score + Prajna priority → combined score)
                   │
          Tasks (shell, kernel services)
                   │
          VGA Shell (interactive, 21 rows)
```

---

## Completed Levels

| Level | Feature | Status |
|---|---|---|
| L1 | Bootloader + VGA text output | ✅ |
| L2 | GDT + IDT + PS/2 keyboard driver | ✅ |
| L3 | Interactive shell + PIT timer (100Hz) + colors | ✅ |
| L4 | ATA disk driver + FAT32 filesystem + classify command | ✅ |
| L5 | Physical memory manager (bitmap allocator) | ✅ |
| L6 | Multitasking — context switch + round-robin scheduler | ✅ |
| L7 | FAT32 shell — ls, cd, cat, touch, write | ✅ |
| L8 | ML inference in kernel space — neural network from disk | ✅ |

---

## Level 9 — AI Kernel Upgrade (Jun–Jul 2026)

### Week 1 — Decision-maker ✅
| Feature | Detail |
|---|---|
| ML score → priority tiers | ai_think() converts ml_score to 0/1/2 priority, scheduler uses combined score |
| Starvation-aware blocking | wait_ticks counter, forced HIGH priority past threshold, reset on switch |
| Anomaly z-score per task | rolling cpu/mem history, deviation check, priority cap on anomaly |
| Harden task_create() | null-check pmm_alloc(), abort cleanly on out-of-memory |

### Week 2 — Conversational JARVIS ✅
| Feature | Detail |
|---|---|
| Ring buffer event log | 20-entry circular buffer, every Prajna decision recorded |
| prajna status | shell command — live system state from Prajna |
| prajna why | shell command — last decision + which tasks were blocked and why |
| prajna log | shell command — last 5 Prajna decisions |
| Boot-time greeting | Prajna reads system health and reports on boot |
| Proactive alerts | Prajna warns unprompted — low memory, starvation, anomaly |
| Live top bar | Row 0: state, memory, uptime, task count — updates every 10s |

### Week 3 — Self-awareness (Jul 4–10) 🔄
- Predictive memory model — warn before hitting ALERT threshold
- `prajna why` explainability trace
- Confidence score
- Self-check watchdog

### Week 4 — Integration & Polish (Jul 11–20) 📋
- Retrain ML model with new features
- Personality pass — consistent Prajna voice
- Stress-test demo
- Level 9 documentation

---

## Prajna Laws (immutable)

```
Law 1: Never harm the system or its data
Law 2: Obey Ravi unless it violates Law 1
Law 3: Protect itself unless it violates Law 1 or 2
Law 4: Never give full control to one task
Law 5: Always maintain a recovery shell
```

---

## ML in Kernel Space

```
PrajnaOS> classify
setosa

PrajnaOS> prajna status
Prajna: CALM - system healthy, all tasks normal

PrajnaOS> prajna why
Last decision: CALM
Blocked tasks: none

PrajnaOS> prajna log
Last 5 Prajna decisions:
CALM
CALM
CALM
CALM
CALM
```

Neural network inference running directly in kernel space —
no OS underneath, no Python, no TensorFlow, no internet.
Weights loaded from FAT32 disk into kernel memory at boot.
Inference runs every 50ms to drive real scheduling decisions.

---

## Screen Layout

```
Row 0   │ PrajnaOS │ CALM │ Mem:130799pg │ Up:340t │ Tasks:1  ← live top bar
Row 1   │ ──────────────────────────────────────────────────  ← divider
Row 2   │                                                      ← shell start
  ...   │   interactive shell (21 rows)
Row 22  │                                                      ← shell end
Row 23  │ [PRAJNA] Warning: ...                               ← proactive alerts
```

---

## Shell Commands

```
help      about     clear     uptime    echo
version   hello     beep      poweroff  reboot
classify  ls        cd        cat       touch
write     prajna status       prajna why
prajna log
```

---

## Kernel Files

```
kernel/
├── kernel.c          — boot sequence, hardware init
├── gdt.c / gdt.asm   — Global Descriptor Table
├── idt.c / idt.asm   — Interrupt Descriptor Table
├── isr.c             — interrupt handlers, keyboard, VGA
├── pit.c             — timer, top bar, scheduler trigger
├── ata.c             — ATA PIO disk driver
├── fat32.c           — FAT32 filesystem driver
├── pmm.c             — physical memory manager
├── scheduler.c       — ML-aware task scheduler
├── heap.c            — kernel heap allocator
├── ai_kernel.c       — Prajna AI core (Sense→Think→Act)
├── klib.c            — kernel stdlib (no external deps)
├── shell.c           — interactive shell + commands
└── ml/
    ├── ml_math.c     — normalize, float utilities
    ├── ml_infer.c    — neural network forward pass
    └── ml_weights.c  — weights loaded from IRIS.BIN
```

---

## Training Dataset

A 5,655-row dataset was generated for Week 4 model retraining:
- 655 rows — real Fedora Linux process samples (collected via systemd service)
- 5,000 rows — synthetic scheduler data (cpu, mem, wait, priority distributions)

Features: `cpu_usage`, `mem_usage`, `wait_time`, `priority` → `score`

---

## Research

**Target:** IEEE ICACCI — *"PrajnaOS: An AI-Centric Bare Metal OS with ML Inference as the Primary Scheduler"*

**Research gap:** All existing ML inference work runs on Linux or requires an existing OS.
PrajnaOS runs a trained neural network directly in kernel space of a custom OS
built entirely from scratch — no stdlib, no OS underneath, no external libraries.

**SIH 2026:** Foundation for India SHAKTI RISC-V edge AI deployment.

---

## Tech Stack

| Component | Detail |
|---|---|
| Language | C + x86 Assembly (NASM) |
| Compiler | GCC -m32 -ffreestanding -fno-stack-protector |
| Emulator | QEMU qemu-system-i386 |
| Bootloader | GRUB2 multiboot |
| Filesystem | FAT32 (custom driver, no external libs) |
| ML | Custom neural network, weights from disk |
| Host OS | Fedora Linux |

---

## Build & Run

```bash
make clean && make && make run
```

---

## Future (Level 10+)

- Stage 2: hardware sensors, keyboard activity, disk health
- Stage 3: pattern memory, adaptive scheduling
- Stage 4: PC speaker, voice/TTS
- Stage 5: RTL8139 NIC, TCP/IP stack, HTTP server
- Stage 6: ELF loader, MicroPython, SHAKTI RISC-V port

---

*Built by Ravi — 2nd year CSE + Data Science, Jaipur, India*  
*"Consciousness. Intelligence. Control."*
