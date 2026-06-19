CC      = gcc
LD      = ld
ASM     = nasm

CFLAGS  = -ffreestanding -m32 -O2 -Wall -Wextra \
          -fno-stack-protector -fno-pic -nostdlib -fno-pie
LDFLAGS = -T kernel/kernel.ld --oformat binary -m elf_i386

ISODIR  = iso

KERNEL_OBJS = kernel/kernel_entry.o \
              kernel/src/kernel.o \
              kernel/src/io.o \
              kernel/src/vga.o \
              kernel/src/string.o \
              kernel/src/keyboard.o \
              kernel/src/fs.o \
              kernel/src/editor.o \
              kernel/src/shell.o

all: myos.img

bootloader.bin: boot/bootloader.asm
	$(ASM) -f bin $< -o $@

kernel/kernel_entry.o: kernel/kernel_entry.asm
	$(ASM) -f elf32 $< -o $@

kernel/src/%.o: kernel/src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

myos.img: bootloader.bin kernel.bin
	dd if=/dev/zero      of=myos.img bs=512 count=2880  2>/dev/null
	dd if=bootloader.bin of=myos.img bs=512 count=1     conv=notrunc 2>/dev/null
	dd if=kernel.bin     of=myos.img bs=512 seek=1      conv=notrunc 2>/dev/null
	@echo "=== myos.img KÉSZ! ==="

run: myos.img
	qemu-system-i386 -drive format=raw,file=myos.img,if=floppy -boot a

run-iso: myos.iso
	qemu-system-i386 -cdrom myos.iso

debug: myos.img
	qemu-system-i386 -drive format=raw,file=myos.img,if=floppy \
	                 -boot a -s -S &
	gdb -ex "target remote :1234" \
	    -ex "set architecture i386"

clean:
	rm -f bootloader.bin kernel.bin myos.img myos.iso
	rm -f kernel/kernel_entry.o kernel/src/*.o
	rm -rf $(ISODIR)

.PHONY: all run run-iso debug clean