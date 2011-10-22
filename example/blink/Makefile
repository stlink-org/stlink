CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

DEF_CFLAGS=-g -O2 -mlittle-endian -mthumb -ffreestanding -nostdlib -nostdinc

# to run from SRAM
DEF_CFLAGS+=-Wl,-Ttext,0x20000000 -Wl,-e,0x20000000

# to write to flash then run
# DEF_CFLAGS+=-Wl,-Ttext,0x08000000 -Wl,-e,0x08000000

CFLAGS_VL=$(DEF_CFLAGS) -mcpu=cortex-m3 -DCONFIG_STM32VL_DISCOVERY=1
CFLAGS_L=$(DEF_CFLAGS) -mcpu=cortex-m3 -DCONFIG_STM32L_DISCOVERY
CFLAGS_F4=$(DEF_CFLAGS) -mcpu=cortex-m4 -DCONFIG_STM32F4_DISCOVERY=1

all: blink_32VL.elf blink_32L.elf blink_F4.elf

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

blink_32VL.elf: main.c
	$(CC) $(CFLAGS_VL) $^ -o $@
blink_32L.elf: main.c
	$(CC) $(CFLAGS_L) $^ -o $@
blink_F4.elf: main.c
	$(CC) $(CFLAGS_F4) $^ -o $@

clean:
	rm -rf *.elf
	rm -rf *.bin

.PHONY: all clean
