# The flash loader code cannot be compiled by the system gcc. This 
# makefile use arm-none-eabi-gcc for this purpose

CROSS_COMPILE ?= arm-none-eabi-

CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

XXD = xxd
XXDFLAGS = -i -c 4 

CFLAGS_ARMV6_M = -mcpu=Cortex-M0 -Tlinker.ld -ffreestanding -nostdlib
CFLAGS_ARMV7_M = -mcpu=Cortex-M3 -Tlinker.ld -ffreestanding -nostdlib

all: stm32vl.h stm32f0.h stm32lx.h stm32f4.h stm32f4lv.h stm32l4.h stm32f7.h stm32f7lv.h
	

%.h: %.bin
	$(XXD) $(XXDFLAGS) $< $@

%.bin: %.o
	$(OBJCOPY) -O binary $< $@
	rm $<

# separate rule for STM32F0
stm32f0.o: stm32f0.s
	$(CC) stm32f0.s $(CFLAGS_ARMV6_M) -o stm32f0.o

# separate rule for STM32F1/F3
stm32vl.o: stm32f0.s
	$(CC) stm32f0.s $(CFLAGS_ARMV7_M) -o stm32vl.o

# generic rule for all other ARMv7-M
%.o: %.s
	$(CC) $< $(CFLAGS_ARMV7_M) -o $@

clean:
	rm -f *.h
