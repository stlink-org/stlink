EXECUTABLE=blink.elf
BIN_IMAGE=blink.bin

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-g -O2 -mlittle-endian -mthumb
ifeq ($(CONFIG_STM32L_DISCOVERY), 1)
	CFLAGS+=-mcpu=cortex-m3 -DCONFIG_STM32L_DISCOVERY
else ifeq ($(CONFIG_STM32VL_DISCOVERY), 1)
	CFLAGS+=-mcpu=cortex-m3 -DCONFIG_STM32VL_DISCOVERY=1
else ifeq ($(CONFIG_STM32F4_DISCOVERY), 1)
	CFLAGS+=-mcpu=cortex-m4 -DCONFIG_STM32F4_DISCOVERY=1	
endif
CFLAGS+=-ffreestanding -nostdlib -nostdinc

# to run from SRAM
CFLAGS+=-Wl,-Ttext,0x20000000 -Wl,-e,0x20000000

# to write to flash then run
# CFLAGS+=-Wl,-Ttext,0x08000000 -Wl,-e,0x08000000

all: $(BIN_IMAGE)

$(BIN_IMAGE): $(EXECUTABLE)
	$(OBJCOPY) -O binary $^ $@

$(EXECUTABLE): main.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(BIN_IMAGE)

.PHONY: all clean
