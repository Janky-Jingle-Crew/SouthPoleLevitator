# Toolchain
CC = arm-none-eabi-gcc
NM = arm-none-eabi-nm
OBJCOPY = arm-none-eabi-objcopy

# Compiler & linker flags
CFLAGS = -mcpu=cortex-m0plus -O2 -mthumb -Wall -ffreestanding \
         -Iinclude -Iinclude/cmsis -Iinclude/stm32 -Iinclude/RTT \
         -DSTM32G030xx -fno-exceptions -fno-unwind-tables \
         -MMD -MP   # <-- generate dependency files
LDFLAGS = -T stm32g030k6tx_FLASH.ld -nostdlib -lc -lgcc -lnosys

# Source files (app + stm32-specific)
SRC = $(wildcard src/*.c) $(wildcard src/*.s) \
      $(wildcard src/stm32/*.c) $(wildcard src/stm32/*.s) \
      $(wildcard src/RTT/*.c) $(wildcard src/RTT/*.s)

# Object files in build/ (preserve subfolders)
OBJ = $(patsubst %.c, build/%.o, $(SRC))
OBJ := $(patsubst %.s, build/%.o, $(OBJ))

# Dependency files
DEP = $(OBJ:.o=.d)

TARGET = build/firmware

# Default rule
all: $(TARGET).bin
	arm-none-eabi-size $(TARGET).elf

# Compile C
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile ASM
build/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET).elf: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

# Convert to binary
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Include automatically generated dependencies
-include $(DEP)

# Clean
clean:
	rm -rf build/*

# Flash with stlink
flash: all
	ST-LINK_CLI.exe -c SWD -P build/firmware.bin 0x08000000 -V -rst

# Flash with pyocd, not working for now, probably needs target specification
# flash: all
# 	python -m pyocd load build\firmware.bin --base-address 0x08000000

# Target to print RTT
rtt:
	python rtt.py
	
asm: all
	arm-none-eabi-objdump -d build/firmware.elf > disasm.txt
