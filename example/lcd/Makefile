ELF=lcd.elf

CC=arm-none-eabi-gcc

CFLAGS=-O2 -mlittle-endian -mthumb -g
CFLAGS+=-mcpu=cortex-m3 -ffreestanding -nostdlib -nostdinc

CFLAGS+=-I.


PLATFORM=stm32l1xx
LIBS_STM_PATH=../libs_stm

INC_CORE_SUPPORT=$(LIBS_STM_PATH)/inc/core_support
SRC_CORE_SUPPORT=$(LIBS_STM_PATH)/inc/core_support
INC_DEVICE_SUPPORT=$(LIBS_STM_PATH)/inc/device_support
INC_PLATFORM=$(LIBS_STM_PATH)/inc/$(PLATFORM)

CFLAGS+=-I$(INC_CORE_SUPPORT)
CFLAGS+=-I$(INC_DEVICE_SUPPORT)
CFLAGS+=-I$(INC_PLATFORM)
CFLAGS+=-I$(LIBS_STM_PATH)/inc/base


# to run from SRAM
CFLAGS+=-Wl,-T,linker_stm32l.lds

SRCS=\
main.c\
stm32l_discovery_lcd.c

OBJS=$(SRCS:.c=.o)

all: $(ELF)

$(ELF): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -L$(LIBS_STM_PATH)/build -lstm32l_discovery

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	-rm -f $(OBJS)
	-rm -f $(ELF)

.PHONY: all clean
