all: myos.iso

kernel.bin:
	nasm -f elf32 kernel/boot.asm -o boot.o
	gcc -m32 -ffreestanding -c kernel/kernel.c -o kernel.o
	ld -m elf_i386 -T linker.ld -o kernel.bin boot.o kernel.o

myos.iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	cp boot/grub/grub.cfg iso/boot/grub/grub.cfg
	grub2-mkrescue -o myos.iso iso

run: myos.iso
	qemu-system-x86_64 -cdrom myos.iso -m 512 -boot d

clean:
	rm -f *.o kernel.bin myos.iso
	rm -rf iso