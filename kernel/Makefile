SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
LINK_SCRIPT = linker.ld
FONTFILE := zap-vga09.psf
FONTOBJ := $(OBJ_DIR)/font.o

SRC_FILES = $(shell find $(SRC_DIR) -name '*.c')
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

CC = x86_64-elf-gcc
CFLAGS = -g -O2 -pipe -Wall -Wextra -std=gnu11 -nostdinc -ffreestanding -fno-stack-protector \
         -fno-stack-check -fno-lto -fno-PIC -ffunction-sections -fdata-sections -m64 \
         -march=x86-64 -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel \
         -I $(SRC_DIR) -isystem $(SRC_DIR)/freestanding-headers
LDFLAGS = -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -gc-sections -T $(LINK_SCRIPT)



all: kernel


kernel: $(OBJ_FILES) $(FONTOBJ)
	mkdir -p $(BIN_DIR)
	ld.lld $^ $(LDFLAGS) -o $(BIN_DIR)/$@


# Create obj directory if it doesn't exist and compile each .c file into .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(OBJ_FILES:.o=.d)


$(FONTOBJ): | $(OBJ_DIR)
	objcopy -O elf64-x86-64 -B i386:x86-64 -I binary $(FONTFILE) $(FONTOBJ)


clean:
	rm -f $(OBJ_FILES) $(OBJ_FILES:.o=.d)
	rm -rf $(OBJ_DIR) $(BIN_DIR)


.PHONY: all clean
