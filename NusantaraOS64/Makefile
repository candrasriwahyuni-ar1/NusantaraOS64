# NusantaraOS64 Build System
# Makefile untuk membangun kernel dan bootloader UEFI

# Compiler settings
CC = gcc
AS = nasm
LD = ld

# Target architecture
ARCH = x86_64

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
LIB_DIR = lib
INCLUDE_DIR = include
BUILD_DIR = build
DIST_DIR = dist

# Output files
KERNEL_BIN = $(DIST_DIR)/kernel.bin
BOOTLOADER_EFI = $(DIST_DIR)/bootx64.efi
ISO_IMAGE = $(DIST_DIR)/nusantara.iso

# Compiler flags
CFLAGS = -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
         -mstackrealign -mabi=sysv -mno-mmx -mno-sse -mno-sse2 \
         -Wall -Wextra -Werror -I$(INCLUDE_DIR) -nostdlib -nodefaultlibs \
         -c -O2 -g

# Assembly flags for NASM
ASFLAGS = -f elf64 -g -F dwarf

# Linker flags
LDFLAGS = -nostdlib -static -z max-page-size=0x1000 -T linker.ld

# Source files
BOOT_ASM = $(wildcard $(BOOT_DIR)/*.S)
KERNEL_C = $(wildcard $(KERNEL_DIR)/*.c)
DRIVERS_C = $(wildcard $(DRIVERS_DIR)/*.c)
LIB_C = $(wildcard $(LIB_DIR)/*.c)

# Object files
BOOT_OBJ = $(patsubst $(BOOT_DIR)/%.S,$(BUILD_DIR)/boot/%.o,$(BOOT_ASM))
KERNEL_OBJ = $(patsubst $(KERNEL_DIR)/%.c,$(BUILD_DIR)/kernel/%.o,$(KERNEL_C))
DRIVERS_OBJ = $(patsubst $(DRIVERS_DIR)/%.c,$(BUILD_DIR)/drivers/%.o,$(DRIVERS_C))
LIB_OBJ = $(patsubst $(LIB_DIR)/%.c,$(BUILD_DIR)/lib/%.o,$(LIB_C))

ALL_OBJ = $(BOOT_OBJ) $(KERNEL_OBJ) $(DRIVERS_OBJ) $(LIB_OBJ)

# Default target
all: dirs $(KERNEL_BIN)
	@echo "=========================================="
	@echo "  NusantaraOS64 Build Complete!"
	@echo "=========================================="
	@echo ""
	@echo "Output files:"
	@echo "  Kernel:    $(KERNEL_BIN)"
	@echo "  ISO Image: $(ISO_IMAGE)"
	@echo ""
	@echo "To run in QEMU:"
	@echo "  make run"
	@echo ""
	@echo "To create bootable USB:"
	@echo "  sudo dd if=$(ISO_IMAGE) of=/dev/sdX bs=4M status=progress"
	@echo ""

# Create build directories
dirs:
	@mkdir -p $(BUILD_DIR)/boot
	@mkdir -p $(BUILD_DIR)/kernel
	@mkdir -p $(BUILD_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/lib
	@mkdir -p $(DIST_DIR)

# Compile boot assembly
$(BUILD_DIR)/boot/%.o: $(BOOT_DIR)/%.S
	@echo "[AS] $<"
	$(AS) $(ASFLAGS) -o $@ $<

# Compile kernel C files
$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.c
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -o $@ $<

# Compile driver C files
$(BUILD_DIR)/drivers/%.o: $(DRIVERS_DIR)/%.c
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -o $@ $<

# Compile library C files
$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/%.c
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -o $@ $<

# Link kernel
$(KERNEL_BIN): $(ALL_OBJ) linker.ld
	@echo "[LD] Creating kernel binary..."
	$(LD) $(LDFLAGS) -o $(BUILD_DIR)/kernel.elf $(ALL_OBJ)
	@echo "[OBJCOPY] Stripping ELF headers..."
	objcopy -O binary $(BUILD_DIR)/kernel.elf $(KERNEL_BIN)
	@echo "[KERNEL] Kernel built successfully!"

# Create bootable ISO image
$(ISO_IMAGE): $(KERNEL_BIN)
	@echo "[ISO] Creating bootable ISO..."
	@mkdir -p $(DIST_DIR)/iso/EFI/BOOT
	@cp $(KERNEL_BIN) $(DIST_DIR)/iso/kernel.bin
	@# Note: For full UEFI boot, you need to compile the bootloader with gnu-efi
	@# cp $(BOOTLOADER_EFI) $(DIST_DIR)/iso/EFI/BOOT/bootx64.efi
	@# For now, create a simple ISO that can be booted with GRUB
	grub-mkrescue -o $(ISO_IMAGE) $(DIST_DIR)/iso 2>/dev/null || \
		xorriso -as mkisofs -o $(ISO_IMAGE) \
			-eltorito-alt-boot \
			-no-emul-boot \
			-isohybrid-gpt-basdat \
			$(DIST_DIR)/iso 2>/dev/null || \
		echo "[WARN] Could not create ISO (install grub-mkrescue or xorriso)"
	@echo "[ISO] ISO created: $(ISO_IMAGE)"

# Run in QEMU
run: $(ISO_IMAGE)
	@echo "[QEMU] Starting NusantaraOS64..."
	qemu-system-x86_64 \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-m 512M \
		-serial stdio \
		-display gtk || \
	qemu-system-x86_64 \
		-drive format=raw,file=$(ISO_IMAGE) \
		-m 512M \
		-serial stdio \
		-nographic

# Run with direct kernel boot (for testing)
run-kernel: $(KERNEL_BIN)
	@echo "[QEMU] Running kernel directly..."
	qemu-system-x86_64 \
		-kernel $(KERNEL_BIN) \
		-m 512M \
		-serial stdio \
		-display gtk

# Debug in QEMU with GDB
debug: $(KERNEL_BIN)
	@echo "[QEMU] Starting QEMU with GDB stub..."
	qemu-system-x86_64 \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-m 512M \
		-serial stdio \
		-s -S &
	@echo "Connect with: gdb -ex 'target remote localhost:1234' $(BUILD_DIR)/kernel.elf"

# Clean build artifacts
clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)
	@echo "[CLEAN] Done!"

# Install GNU-EFI development files (for UEFI bootloader)
install-efi-deps:
	@echo "[APT] Installing EFI dependencies..."
	sudo apt-get update
	sudo apt-get install -y gnu-efi gnu-efi-dev

# Build UEFI bootloader (requires gnu-efi)
efi: install-efi-deps dirs
	@echo "[EFI] Building UEFI bootloader..."
	$(CC) -I/usr/include/efi -I/usr/include/efi/$(ARCH) \
		-I/usr/include/efi/protocol \
		-fPIC -ffreestanding -fshort-wchar -mno-red-zone \
		-DEFI_FUNCTION_WRAPPER \
		-c -o $(BUILD_DIR)/uefi_boot.o $(BOOT_DIR)/uefi_boot.c
	$(LD) -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds \
		-shared -Bsymbolic -L /usr/lib \
		$(BUILD_DIR)/uefi_boot.o -o $(BOOTLOADER_EFI) \
		-lc -lefi -lgnuefi
	@objcopy -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym -j .rel -j .rela -j .reloc \
		--target=efi-app-$(ARCH) $(BOOTLOADER_EFI)
	@echo "[EFI] Bootloader built: $(BOOTLOADER_EFI)"

# Show help
help:
	@echo "NusantaraOS64 Build System"
	@echo "=========================="
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build kernel and ISO (default)"
	@echo "  run      - Build and run in QEMU"
	@echo "  run-kernel - Run kernel directly in QEMU"
	@echo "  debug    - Run in QEMU with GDB stub"
	@echo "  efi      - Build UEFI bootloader"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - gcc (x86_64 cross-compiler recommended)"
	@echo "  - nasm"
	@echo "  - qemu-system-x86"
	@echo "  - grub-mkrescue or xorriso (for ISO creation)"
	@echo "  - gnu-efi (for UEFI bootloader)"
	@echo ""

.PHONY: all dirs run run-kernel debug clean efi help install-efi-deps
