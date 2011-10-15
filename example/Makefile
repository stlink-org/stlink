EXECUTABLE=blink.elf
BIN_IMAGE=blink.bin

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-O2 -mlittle-endian -mthumb
CFLAGS+=-mcpu=cortex-m3 -ffreestanding -nostdlib -nostdinc

# to run from SRAM
# CFLAGS+=-Wl,-Ttext,0x20000000

# to write to flash then run
CFLAGS+=-Wl,-Ttext,0x08000000

all: $(BIN_IMAGE)

$(BIN_IMAGE): $(EXECUTABLE)
	$(OBJCOPY) -O binary $^ $@

$(EXECUTABLE): main.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(BIN_IMAGE)

.PHONY: all clean
