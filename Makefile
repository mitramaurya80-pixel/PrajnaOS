CC      = gcc
CFLAGS  = -m32 -ffreestanding -Ikernel/include -fno-stack-protector
LDFLAGS = -m elf_i386 -T linker.ld

OBJS = boot.o gdt_asm.o gdt.o idt_asm.o idt.o isr.o kernel.o shell.o pit.o ata.o ml_math.o

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

run: myos.iso disk.img
	# remount and update kernel on disk
	mkdir -p /tmp/prajna
	sudo losetup -P /dev/loop10 disk.img
	sudo mount /dev/loop10p1 /tmp/prajna
	sudo cp iso/boot/kernel.bin /tmp/prajna/boot/
	sudo umount /tmp/prajna
	sudo losetup -d /dev/loop10
	# boot from disk
	qemu-system-i386 \
	-drive file=disk.img,format=raw,if=ide,bus=0,unit=0 \
	-m 512 -boot c
	

clean:
	rm -f *.o kernel.bin myos.iso
	rm -rf iso
