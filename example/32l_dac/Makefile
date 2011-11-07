EXECUTABLE=dac.elf
BIN_IMAGE=dac.bin

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-O3 -mlittle-endian -mthumb
ifeq ($(CONFIG_STM32L_DISCOVERY), 1)
	CFLAGS+=-mcpu=cortex-m3 -DCONFIG_STM32L_DISCOVERY=1
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

# stm32l_discovery lib
CFLAGS+=-I../libstm32l_discovery/inc
CFLAGS+=-I../libstm32l_discovery/inc/base
CFLAGS+=-I../libstm32l_discovery/inc/core_support
CFLAGS+=-I../libstm32l_discovery/inc/device_support

all: $(BIN_IMAGE)

$(BIN_IMAGE): $(EXECUTABLE)
	$(OBJCOPY) -O binary $^ $@

$(EXECUTABLE): main.c system_stm32l1xx.c startup_stm32l1xx_md.s
	$(CC) $(CFLAGS) $^ -o $@  -L../libstm32l_discovery/build -lstm32l_discovery

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(BIN_IMAGE)

write: all
	sudo ../../flash/flash write ./dac.bin 0x08000000

.PHONY: all clean write
