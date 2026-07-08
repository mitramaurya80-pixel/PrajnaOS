CC      = gcc
CFLAGS  = -m32 -ffreestanding -Ikernel/include \
           -fno-stack-protector -mfpmath=387 \
           -MMD -MP
LDFLAGS = -m elf_i386 -T linker.ld

OBJS = \
boot.o gdt_asm.o gdt.o idt_asm.o idt.o isr.o kernel.o \
shell.o pit.o ata.o fat32.o pmm.o scheduler.o switch.o \
heap.o klib.o ai_kernel.o \
ml_math.o ml_infer.o ml_weights.o

DEPS = $(OBJS:.o=.d)

all: myos.iso

# -----------------------------
# Assembly files
# -----------------------------
boot.o: kernel/boot.asm
	nasm -f elf32 $< -o $@

gdt_asm.o: kernel/gdt.asm
	nasm -f elf32 $< -o $@

idt_asm.o: kernel/idt.asm
	nasm -f elf32 $< -o $@

switch.o: kernel/switch.asm
	nasm -f elf32 $< -o $@

#-----------------------------
# C files
#-----------------------------
%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@
#-----------------------------
# ml files
#-----------------------------
ml_math.o: kernel/ml/ml_math.c
	$(CC) $(CFLAGS) -c $< -o $@

ml_infer.o: kernel/ml/ml_infer.c
	$(CC) $(CFLAGS) -c $< -o $@

ml_weights.o: kernel/ml/ml_weights.c
	$(CC) $(CFLAGS) -c $< -o $@

# -----------------------------
# Link kernel
# -----------------------------

kernel.bin: $(OBJS)
	ld $(LDFLAGS) -o $@ $(OBJS)

# -----------------------------
# Build ISO
# -----------------------------

myos.iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	cp boot/grub/grub.cfg iso/boot/grub/grub.cfg
	grub2-mkrescue -o $@ iso

# -----------------------------
# Setup
# -----------------------------

setup: myos.iso
	@bash setup.sh

# -----------------------------
# Run
# -----------------------------

run: myos.iso disk.img
	qemu-system-i386 \
	-drive file=disk.img,format=raw,if=ide,bus=0,unit=0 \
	-cdrom myos.iso \
	-m 512 \
	-boot c

# -----------------------------
# Clean
# -----------------------------

clean:
	rm -f *.o *.d kernel.bin myos.iso
	rm -rf iso

# Automatically include generated dependency files
-include $(DEPS)