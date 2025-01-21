# Compiler and tools
CC = arm-eabi-gcc
OBJCOPY = arm-eabi-objcopy

# Flags
FLAGS = -mthumb -mcpu=cortex-m4
CPPFLAGS = -DSTM32F411xE
CFLAGS = $(FLAGS) -Wall -g \
    -O2 -ffunction-sections -fdata-sections \
    -I/opt/arm/stm32/inc \
    -I/opt/arm/stm32/CMSIS/Include \
    -I/opt/arm/stm32/CMSIS/Device/ST/STM32F4xx/Include \
    -MMD -MP
LDFLAGS = $(FLAGS) -Wl,--gc-sections -nostartfiles \
    -L/opt/arm/stm32/lds -Tstm32f411re.lds

# Output directory for all build artifacts
BUILD_DIR = build

# Source files (automatically detect all .c files in the current directory)
SOURCES = $(wildcard *.c)

# Object files (place them in the build directory)
OBJECTS = $(BUILD_DIR)/startup_stm32.o $(BUILD_DIR)/delay.o $(BUILD_DIR)/gpio.o $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Dependency files (also in the build directory)
DEPS = $(OBJECTS:.o=.d)

# Target name
TARGET = adc

# Default target: build the binary
all: $(TARGET).bin | $(BUILD_DIR)

# Ensure the build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/startup_stm32.o: | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c /opt/arm/stm32/src/startup_stm32.c -o $@

$(BUILD_DIR)/delay.o: | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c /opt/arm/stm32/src/delay.c -o $@

$(BUILD_DIR)/gpio.o: | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c /opt/arm/stm32/src/gpio.c -o $@

# Rule to compile .c files into .o files in the build directory
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Rule to link object files into an ELF file in the build directory
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

# Rule to convert the ELF file into a binary
$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(OBJCOPY) $< $@ -O binary

# Clean up all build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Include dependency files
-include $(DEPS)

$(info SOURCES: $(SOURCES))
$(info OBJECTS: $(OBJECTS))
$(info DEPS: $(DEPS))
