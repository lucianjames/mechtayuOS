# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
LINK_SCRIPT = linker.ld
FONTFILE := zap-vga09.psf
FONTOBJ := $(OBJ_DIR)/font.o

# Find source files
SRC_FILES_C = $(shell find $(SRC_DIR) -name '*.c')
SRC_FILES_NASM = $(shell find $(SRC_DIR) -name '*.nas')
OBJ_FILES_C = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES_C))
OBJ_FILES_NASM = $(patsubst $(SRC_DIR)/%.nas,$(OBJ_DIR)/%.o,$(SRC_FILES_NASM))

# Compiler and flags
CC = x86_64-elf-gcc
CFLAGS = -g -pipe -Wall -Wextra -std=gnu11 -nostdinc -ffreestanding -fno-stack-protector \
         -fno-stack-check -fno-lto -fno-PIC -ffunction-sections -fdata-sections -m64 \
         -march=x86-64 -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel \
         -I $(SRC_DIR) -isystem $(SRC_DIR)/freestanding-headers
LDFLAGS = -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -gc-sections -T $(LINK_SCRIPT)



all: kernel

kernel: $(OBJ_FILES_C) $(OBJ_FILES_NASM) $(FONTOBJ)
	mkdir -p $(BIN_DIR)
	ld.lld $^ $(LDFLAGS) -o $(BIN_DIR)/$@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.nas
	@mkdir -p $(dir $@)
	nasm -f elf64 $< -o $@

-include $(OBJ_FILES_C:.o=.d)

$(FONTOBJ): | $(OBJ_DIR)
	objcopy -O elf64-x86-64 -B i386:x86-64 -I binary $(FONTFILE) $(FONTOBJ)

clean:
	rm -f $(OBJ_FILES_C) $(OBJ_FILES_NASM) $(OBJ_FILES_C:.o=.d)
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
