EXECUTABLE=blink.elf
BIN_IMAGE=blink.bin

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-O2 -mlittle-endian -mthumb

CFLAGS=-g -O2 -mlittle-endian -mthumb
ifeq ($(CONFIG_STM32L_DISCOVERY), 1)
	CFLAGS+=-mcpu=cortex-m3 -DCONFIG_STM32L_DISCOVERY
else ifeq ($(CONFIG_STM32VL_DISCOVERY), 1)
	CFLAGS+=-mcpu=cortex-m3 -DCONFIG_STM32VL_DISCOVERY=1
else ifeq ($(CONFIG_STM32F4_DISCOVERY), 1)
	CFLAGS+=-mcpu=cortex-m4 -DCONFIG_STM32F4_DISCOVERY=1	
else
$(error "must specify CONFIG_ for board!")
endif
	CFLAGS+=-ffreestanding -nostdlib -nostdinc

# to run from FLASH
CFLAGS+=-Wl,-T,stm32_flash.ld

all: $(BIN_IMAGE)

$(BIN_IMAGE): $(EXECUTABLE)
	$(OBJCOPY) -O binary $^ $@

$(EXECUTABLE): main.c startup_stm32l1xx_md.s
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(BIN_IMAGE)

.PHONY: all clean
