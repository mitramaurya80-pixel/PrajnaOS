CC      = gcc
CFLAGS  = -m32 -ffreestanding -Ikernel/include -fno-stack-protector -mfpmath=387
LDFLAGS = -m elf_i386 -T linker.ld

OBJS = boot.o gdt_asm.o gdt.o idt_asm.o idt.o isr.o kernel.o shell.o pit.o ata.o ml_math.o fat32.o pmm.o scheduler.o switch.o heap.o

all: myos.iso

boot.o:
	nasm -f elf32 kernel/boot.asm -o boot.o

gdt_asm.o:
	nasm -f elf32 kernel/gdt.asm -o gdt_asm.o
switch.o:
	nasm -f elf32 kernel/switch.asm -o switch.o
heap.o:
	$(CC) $(CFLAGS) -c kernel/heap.c -o heap.o

gdt.o:
	$(CC) $(CFLAGS) -c kernel/gdt.c -o gdt.o

idt_asm.o:
	nasm -f elf32 kernel/idt.asm -o idt_asm.o
pmm.o:
	$(CC) $(CFLAGS) -c kernel/pmm.c -o pmm.o

idt.o:
	$(CC) $(CFLAGS) -c kernel/idt.c -o idt.o
fat32.o:
	$(CC) $(CFLAGS) -c kernel/fat32.c -o fat32.o

scheduler.o:
	$(CC) $(CFLAGS) -c kernel/scheduler.c -o scheduler.o

isr.o:
	$(CC) $(CFLAGS) -c kernel/isr.c -o isr.o

kernel.o:
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o

shell.o:
	$(CC) $(CFLAGS) -c kernel/shell.c -o shell.o

pit.o:
	$(CC) $(CFLAGS) -c kernel/pit.c -o pit.o

ata.o:
	$(CC) $(CFLAGS) -c kernel/ata.c -o ata.o

ml_math.o:
	$(CC) $(CFLAGS) -c kernel/ml/ml_math.c -o ml_math.o

kernel.bin: $(OBJS)
	ld $(LDFLAGS) -o kernel.bin $(OBJS)

myos.iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	cp boot/grub/grub.cfg iso/boot/grub/grub.cfg
	grub2-mkrescue -o myos.iso iso

setup: myos.iso
	@bash setup.sh

run: myos.iso disk.img
	qemu-system-i386 \
	-drive file=disk.img,format=raw,if=ide,bus=0,unit=0 \
	-cdrom myos.iso \
	-m 512 -boot c
clean:
	rm -f *.o kernel.bin myos.iso
	rm -rf iso
