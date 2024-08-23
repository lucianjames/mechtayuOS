KERNEL_DIR = kernel
LIMINE_DIR = limine
ISO_DIR = iso_root
BOOT_DIR = $(ISO_DIR)/boot
LIMINE_BOOT_DIR = $(BOOT_DIR)/limine
EFI_DIR = $(ISO_DIR)/EFI/BOOT
ISO_NAME = mechtayu.iso

KERNEL_BINARY = $(KERNEL_DIR)/bin/kernel
LIMINE_FILES = $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin $(LIMINE_DIR)/limine-uefi-cd.bin
EFI_FILES = $(LIMINE_DIR)/BOOTX64.EFI $(LIMINE_DIR)/BOOTIA32.EFI
LIMINE_CONF = limine.conf

all: iso

iso: $(KERNEL_BINARY) $(LIMINE_FILES) $(EFI_FILES) $(ISO_NAME)

$(KERNEL_BINARY):
	$(MAKE) -C $(KERNEL_DIR)

# Force re-build of the kernel on every make
.PHONY: $(KERNEL_BINARY)

$(LIMINE_FILES) $(EFI_FILES):
	$(MAKE) -C $(LIMINE_DIR)

$(ISO_NAME): $(KERNEL_BINARY) $(LIMINE_FILES) $(EFI_FILES)
	mkdir -p $(BOOT_DIR) $(LIMINE_BOOT_DIR) $(EFI_DIR)
	cp -v $(KERNEL_BINARY) $(BOOT_DIR)/
	cp -v $(LIMINE_CONF) $(LIMINE_BOOT_DIR)/
	cp -v $(LIMINE_FILES) $(LIMINE_BOOT_DIR)/
	cp -v $(EFI_FILES) $(EFI_DIR)/
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISO_DIR) -o $(ISO_NAME)

runvm: iso
	qemu-system-x86_64 -boot d -cdrom $(ISO_NAME) -serial file:serial.log -monitor stdio -vga std -m 6144
runvmgdb: iso
	qemu-system-x86_64 -s -S -boot d -cdrom $(ISO_NAME) -serial file:serial.log -monitor stdio -vga std -d cpu_reset -m 6144

clean:
	rm -r $(ISO_DIR) $(ISO_NAME)
	$(MAKE) -C $(KERNEL_DIR) clean
	$(MAKE) -C $(LIMINE_DIR) clean

.PHONY: all iso runvm runvmgdb clean
