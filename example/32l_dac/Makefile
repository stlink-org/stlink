EXECUTABLE=dac.elf
BIN_IMAGE=dac.bin

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-O3 -mlittle-endian -mthumb
CFLAGS+=-mcpu=cortex-m3 -DCONFIG_STM32L_DISCOVERY=1
CFLAGS+=-ffreestanding -nostdlib -nostdinc

# to run from FLASH
CFLAGS+=-Wl,-T,stm32_flash.ld

PLATFORM=stm32l1xx
LIBS_STM_PATH=../libs_stm

# stm32l_discovery lib
CFLAGS+=-I$(LIBS_STM_PATH)/inc/base
CFLAGS+=-I$(LIBS_STM_PATH)/inc/core_support
CFLAGS+=-I$(LIBS_STM_PATH)/inc/device_support
CFLAGS+=-I$(LIBS_STM_PATH)/inc/$(PLATFORM)

LDFLAGS+=-L$(LIBS_STM_PATH)/build -lstm32_stdperiph_l1xx

all: $(BIN_IMAGE)

$(BIN_IMAGE): $(EXECUTABLE)
	$(OBJCOPY) -O binary $^ $@

$(EXECUTABLE): main.c system_stm32l1xx.c startup_stm32l1xx_md.s
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(BIN_IMAGE)

write: all
	sudo ../../flash/flash write ./dac.bin 0x08000000

.PHONY: all clean write
