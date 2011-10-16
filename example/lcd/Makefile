ELF=lcd.elf

CC=arm-none-eabi-gcc

CFLAGS=-O2 -mlittle-endian -mthumb
CFLAGS+=-mcpu=cortex-m3 -ffreestanding -nostdlib -nostdinc

CFLAGS+=-I.

# stm32l_discovery lib
CFLAGS+=-I../libstm32l_discovery/inc
CFLAGS+=-I../libstm32l_discovery/inc/base
CFLAGS+=-I../libstm32l_discovery/inc/core_support
CFLAGS+=-I../libstm32l_discovery/inc/device_support

# to run from SRAM
CFLAGS+=-Wl,-T,linker_stm32l.lds

SRCS=\
main.c\
stm32l_discovery_lcd.c

OBJS=$(SRCS:.c=.o)

all: $(ELF)

$(ELF): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -L../libstm32l_discovery/build -lstm32l_discovery

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	-rm -f $(OBJS)
	-rm -f $(ELF)

.PHONY: all clean
