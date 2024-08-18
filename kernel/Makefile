SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
LINK_SCRIPT = linker.ld

CC = x86_64-elf-gcc
CFLAGS = -g -O2 -pipe -Wall -Wextra -std=gnu11 -nostdinc -ffreestanding -fno-stack-protector \
         -fno-stack-check -fno-lto -fno-PIC -ffunction-sections -fdata-sections -m64 \
         -march=x86-64 -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone -mcmodel=kernel \
         -I $(SRC_DIR) -isystem $(SRC_DIR)/freestanding-headers
LDFLAGS = -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -gc-sections -T $(LINK_SCRIPT)


# Find all C source files in the src directory
SRC_FILES = $(shell find $(SRC_DIR) -name '*.c')
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Define targets
all: kernel

kernel: $(OBJ_FILES)
	mkdir -p $(BIN_DIR)
	ld.lld $^ $(LDFLAGS) -o $(BIN_DIR)/$@

# Create obj directory if it doesn't exist and compile each .c file into .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Handle dependencies
-include $(OBJ_FILES:.o=.d)

# Clean up object files and kernel
clean:
	rm -f $(OBJ_FILES) $(OBJ_FILES:.o=.d)
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Phony targets
.PHONY: all clean
