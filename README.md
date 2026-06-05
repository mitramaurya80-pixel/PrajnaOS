PrajnaOS is a bare metal operating system kernel built 
from scratch in C and x86 Assembly by a 2nd year CSE 
student.

Named after the Sanskrit word for highest intelligence,
PrajnaOS is designed with AI as its central controller —
not as an app on top, but as the core of the OS itself.

Current features:
- Custom GRUB bootloader
- GDT and IDT implementation
- Keyboard driver via IRQ1
- Interactive shell with commands
- PIT timer at 100Hz
- VGA color support
- Blinking cursor

Planned:
- FAT32 filesystem
- Multitasking
- ML inference inside kernel (Level 8)
- Sanskrit NLP voice interface

Built on Fedora Linux using NASM, GCC, and QEMU.

