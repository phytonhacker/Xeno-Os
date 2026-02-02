ASM = nasm
ISODIR = iso
BOOTDIR = $(ISODIR)/boot
GRUBDIR = $(BOOTDIR)/grub

all: myos.iso

boot.bin: boot/bootloader.asm
	$(ASM) -f bin $< -o $@

kernel.bin: kernel/kernel.asm
	$(ASM) -f bin $< -o $@

myos.iso: boot.bin kernel.bin
	# ISO könyvtárstruktúra létrehozása
	mkdir -p $(GRUBDIR)
	
	# Bootolható floppy image készítése
	dd if=/dev/zero of=floppy.img bs=512 count=2880 2>/dev/null
	dd if=boot.bin of=floppy.img bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=kernel.bin of=floppy.img bs=512 seek=1 conv=notrunc 2>/dev/null
	
	# Floppy image az ISO-ba
	cp floppy.img $(ISODIR)/
	
	# El Torito boot catalog config
	@echo 'set timeout=0' > $(GRUBDIR)/grub.cfg
	@echo 'set default=0' >> $(GRUBDIR)/grub.cfg
	
	# ISO készítése mkisofs-sel
	mkisofs -o myos.iso -b floppy.img -c boot.cat $(ISODIR)
	
	@echo "=== myos.iso KÉSZ! ==="
	@ls -lh myos.iso

run: myos.iso
	qemu-system-i386 -cdrom myos.iso

clean:
	rm -f *.bin *.img *.iso
	rm -rf $(ISODIR)