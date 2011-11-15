# build an elf from the an3268 demonstration code
APP=32vl_factory_demo

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-O2 -mlittle-endian -mthumb
CFLAGS+=-mcpu=cortex-m3 -ffreestanding -nostdlib -nostdinc
ifeq ($(DEBUG),1)
	CFLAGS+=-g
endif

# to run from SRAM
CFLAGS+=-Wl,-Ttext,0x20000000 -Wl,-e,0x20000000

# to write to flash then run
# CFLAGS+=-Wl,-Ttext,0x08000000 -Wl,-e,0x08000000

PLATFORM=stm32f10x
BOARD_SUPPORT=../board_support/discovery_32vl
LIBS_STM_PATH=../libs_stm

CFLAGS+=-I.
CFLAGS+=-I$(LIBS_STM_PATH)/inc/base
CFLAGS+=-I$(LIBS_STM_PATH)/inc/core_support
CFLAGS+=-I$(LIBS_STM_PATH)/inc/device_support
CFLAGS+=-I$(LIBS_STM_PATH)/inc/$(PLATFORM)
CFLAGS+=-I$(BOARD_SUPPORT)

LDFLAGS+=-L$(LIBS_STM_PATH)/build -lstm32_stdperiph_f10x


SRCS=$(wildcard *.c)
SRCS+=$(wildcard $(BOARD_SUPPORT)/*.c)

OBJS=$(SRCS:.c=.o)

all: $(APP).elf

%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

$(APP).elf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rf $(OBJS)
	rm -rf $(APP).*

.PHONY: all clean
