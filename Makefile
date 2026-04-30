CC      = gcc
CFLAGS  = -m32 -ffreestanding -Ikernel/include -fno-stack-protector
LDFLAGS = -m elf_i386 -T linker.ld

OBJS = boot.o gdt_asm.o gdt.o idt_asm.o idt.o isr.o kernel.o shell.o pit.o

all: myos.iso

boot.o:
	nasm -f elf32 kernel/boot.asm -o boot.o

gdt_asm.o:
	nasm -f elf32 kernel/gdt.asm -o gdt_asm.o

gdt.o:
	$(CC) $(CFLAGS) -c kernel/gdt.c -o gdt.o

idt_asm.o:
	nasm -f elf32 kernel/idt.asm -o idt_asm.o

idt.o:
	$(CC) $(CFLAGS) -c kernel/idt.c -o idt.o

isr.o:
	$(CC) $(CFLAGS) -c kernel/isr.c -o isr.o

kernel.o:
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o

shell.o:
	$(CC) $(CFLAGS) -c kernel/shell.c -o shell.o

pit.o:
	$(CC) $(CFLAGS) -c kernel/pit.c -o pit.o

kernel.bin: $(OBJS)
	ld $(LDFLAGS) -o kernel.bin $(OBJS)

myos.iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	cp boot/grub/grub.cfg iso/boot/grub/grub.cfg
	grub2-mkrescue -o myos.iso iso

run: myos.iso
	qemu-system-i386 -cdrom myos.iso -m 512 -boot d

clean:
	rm -f *.o kernel.bin myos.iso
	rm -rf iso
