ELF=lcd.elf

CC=arm-none-eabi-gcc

CFLAGS=-O2 -mlittle-endian -mthumb -g
CFLAGS+=-mcpu=cortex-m3 -ffreestanding -nostdlib -nostdinc

CFLAGS+=-I.


PLATFORM=stm32l1xx
LIBS_STM_PATH=../libs_stm

CFLAGS+=-I$(LIBS_STM_PATH)/inc/base
CFLAGS+=-I$(LIBS_STM_PATH)/inc/core_support
CFLAGS+=-I$(LIBS_STM_PATH)/inc/device_support
CFLAGS+=-I$(LIBS_STM_PATH)/inc/$(PLATFORM)

# to run from SRAM
CFLAGS+=-Wl,-T,linker_stm32l.lds

LDFLAGS+=-L$(LIBS_STM_PATH)/build -lstm32_stdperiph_l1xx

SRCS=\
main.c\
stm32l_discovery_lcd.c

OBJS=$(SRCS:.c=.o)

all: $(ELF)

$(ELF): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	-rm -f $(OBJS)
	-rm -f $(ELF)

.PHONY: all clean
