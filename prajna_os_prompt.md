# PrajnaOS — Context Prompt
> Paste this into a new Claude chat to continue PrajnaOS development

## Identity
- **Name:** Ravi
- **Year:** 2nd year CSE + Data Science, Tier 3 college, India
- **OS:** Fedora Linux, zsh, 16GB RAM
- **Editor:** VS Code
- **Tagline:** "Consciousness. Intelligence. Control."

## System Specs
- Compiler: `gcc -m32 -ffreestanding -fno-stack-protector`
- Assembler: `nasm -f elf32`
- Emulator: QEMU `qemu-system-i386`
- No stdlib — uses custom `types.h` instead of `stdint.h`
- Path: `/home/vega/projects/PrajnaOS/`

## Project Structure
```
kernel/
├── boot.asm          — multiboot bootloader
├── kernel.c          — kernel_main()
├── gdt.c + gdt.asm   — GDT setup
├── idt.c + idt.asm   — IDT setup
├── isr.c             — interrupt handlers + keyboard + VGA
├── shell.c           — shell with commands
├── pit.c             — timer (100ms ticks)
├── ata.c             — ATA disk driver
├── fat32.c           — FAT32 filesystem
├── ml/ml_math.c      — ML math (started)
└── include/
    ├── types.h
    ├── gdt.h, idt.h, shell.h, pit.h, ata.h, fat32.h
```

## QEMU Command
```bash
qemu-system-i386 \
  -drive file=disk.img,format=raw,if=ide,bus=0,unit=0 \
  -cdrom myos.iso \
  -m 512 -boot c
```

## Makefile Summary
```makefile
OBJS = boot.o gdt_asm.o gdt.o idt_asm.o idt.o isr.o kernel.o \
       shell.o pit.o ata.o ml_math.o fat32.o
CFLAGS = -m32 -ffreestanding -Ikernel/include -fno-stack-protector -mfpmath=387
```

## Disk Setup
- `disk.img` — 64MB FAT32, partition starts at LBA 2048
- GRUB installed on disk.img
- `grub.cfg` uses `(hd0,msdos1)/boot/kernel.bin`
- Setup script: `bash setup.sh` (mounts, copies kernel + grub + TEST.TXT)

## Completed Levels

### L1 — Bootloader + VGA ✅
- Multiboot compliant bootloader
- VGA text mode output
- Custom `put_char()` with color support

### L2 — GDT + IDT + Keyboard ✅
- GDT with code/data segments
- IDT with 256 interrupt handlers
- PS/2 keyboard input via IRQ1

### L3 — Shell + PIT + Colors + Cursor ✅
- Shell commands: help, about, clear, uptime, echo, version, hello, beep, poweroff, reboot, classify
- PIT timer at 100ms ticks
- Cursor disabled
- Color VGA output

### L4 — ATA + FAT32 ✅ (partially)
- ATA init, read sector, write sector — all working
- Write test PASSED (wrote PRAJNA to sector 1, read back)
- FAT32 init working — reads boot sector at LBA 2048
- FAT32 directory scan working — finds BOOT folder and TEST.TXT
- FAT32 direct read working — reads file content correctly
- **BUG:** `fat32_find_file()` name match not working — file found in scan but match returns 0
- **Workaround:** `fat32_read_test()` reads TEST.TXT directly by hardcoded offset — WORKS

### ML Seed — classify command ✅
- FPU enabled in kernel_main
- `classify_iris()` in shell.c — hardcoded decision tree
- Shell command `classify` → prints "setosa"
- No stdlib, no malloc, pure C float arithmetic

## Current Bug — fat32_find_file
```
Symptom: Directory scan finds [BOOT][TEST.TXT] but returns "File not found"
Debug done:
  - Inline byte comparison shows all Y (name matches)
  - Ext comparison shows all Y (ext matches)
  - But match variable never triggers return 0
  - fat32_read_test() reads file directly — proves data is correct
  - Raw sector 4066 xxd confirms TEST.TXT at offset 0x40
Next step: Print cluster value from TEST.TXT entry, verify LBA calculation
```

## Planned Levels
```
L5 — Memory Manager     → bitmap page allocator (pmm.c)
L6 — Multitasking       → process table, round-robin scheduler
L7 — FAT32 complete     → file write, create, delete
L8 — ML Inference       → Iris classifier loading weights from FAT32
```

## Mini Project — IEEE ICACCI Target
- **Title:** Neural Network Inference Engine in Bare Metal OS Kernel
- **Research gap:** All existing work runs ML on Linux — not in kernel space
- **Phase 1:** Math library (no stdlib) — ml_math.c started
- **Phase 2:** Weight loader from FAT32 disk
- **Phase 3:** Inference engine (forward pass)
- **Phase 4:** Scheduler integration

## Shell Commands Available
| Command | Function |
|---|---|
| help | list commands |
| about | OS info |
| clear | clear screen |
| uptime | show PIT ticks |
| echo \<text\> | print text |
| version | OS version |
| hello | hello message |
| beep | bell character |
| poweroff | halt CPU |
| reboot | keyboard controller reset |
| classify | run Iris classifier → prints setosa |

## Preferences
- Line by line comments in ALL code
- No stdlib anywhere in OS code
- Explain simply
- Build step by step
- Use `types.h` not `stdint.h`

## Start Here
Ask me to paste current `fat32.c` and help fix the `fat32_find_file` name match bug.
The direct read works — only the search function is broken.
