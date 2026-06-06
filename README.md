# PrajnaOS

> *Consciousness. Intelligence. Control.*

PrajnaOS is a bare metal operating system kernel built from scratch 
in C and x86 Assembly by a 2nd year CSE + Data Science student 
at a tier 3 college in India.

Named after the Sanskrit word for highest intelligence, PrajnaOS 
is designed with AI as its central controller — not as an app on 
top of an OS, but running directly in kernel space with no Linux, 
no stdlib, no external dependencies.

---

## Completed

| Level | Feature | Status |
|---|---|---|
| L1 | Bootloader + VGA text output | ✅ |
| L2 | GDT + IDT + PS/2 Keyboard driver | ✅ |
| L3 | Interactive shell + PIT timer (100Hz) + colors | ✅ |
| L4 | ATA disk driver + FAT32 filesystem + ML seed | ✅ |
| L5 | Physical memory manager (bitmap allocator) | ✅ |

## In Progress

| Level | Feature | Status |
|---|---|---|
| L6 | Multitasking — round-robin scheduler | 🔄 |

## Planned

| Level | Feature |
|---|---|
| L7 | FAT32 file write + create |
| L8 | ML inference engine in kernel space |

---

## ML in Kernel Space (L4 seed)

```
PrajnaOS> classify
setosa
```

Iris flower classification running directly in kernel space —
no OS, no Python, no TensorFlow, no internet.
Decision tree implemented in pure C with FPU enabled manually.
This is the foundation for L8 full neural network inference.

---

## Research Gap

All existing work on ML inference runs on Linux or requires
an existing OS. PrajnaOS runs ML directly in kernel space
of a custom OS built from scratch — no stdlib, no external
libraries, no operating system underneath.

Target: IEEE ICACCI paper + Smart India Hackathon 2026

---

## Shell Commands

`help` `about` `clear` `uptime` `echo` `version`
`hello` `beep` `poweroff` `reboot` `classify`

---

## Tech Stack

- **Language:** C + x86 Assembly (NASM)
- **Compiler:** GCC -m32 -ffreestanding
- **Emulator:** QEMU qemu-system-i386
- **Bootloader:** GRUB2 multiboot
- **Filesystem:** FAT32 (custom driver)
- **Host OS:** Fedora Linux

---

## Build & Run

```bash
make clean && make && make run
```

---

*Built by Ravi — 2nd year CSE student, India*  
*"Consciousness. Intelligence. Control."*
