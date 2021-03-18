# Note that according to the original GPLed code, compiling is noted to be
# as simple as gcc -c, this fails with my tests where this will lead to a wrong 
# address read by the program.
# This makefile will save your time from dealing with compile errors
# Adjust CC if needed

CROSS_COMPILE ?= /opt/gcc-arm-none-eabi-6-2017-q2-update/bin/arm-none-eabi-

CC = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy

XXD = xxd
XXDFLAGS = -i -c 4 

CFLAGS_THUMB1 = -mcpu=Cortex-M0 -Tlinker.ld -ffreestanding -nostdlib
CFLAGS_THUMB2 = -mcpu=Cortex-M3 -Tlinker.ld -ffreestanding -nostdlib

all: stm32vl.h stm32f0.h stm32lx.h stm32f4.h stm32f4lv.h stm32l4.h stm32f7.h stm32f7lv.h

stm32f0.h: stm32f0.s
	$(CC) stm32f0.s $(CFLAGS_THUMB1) -o stm32f0.o
	$(OBJCOPY) -O binary stm32f0.o stm32f0.bin
	$(XXD) $(XXDFLAGS) stm32f0.bin stm32f0.h

stm32vl.h: stm32f0.s
	$(CC) stm32f0.s $(CFLAGS_THUMB2) -o stm32vl.o
	$(OBJCOPY) -O binary stm32vl.o stm32vl.bin
	$(XXD) $(XXDFLAGS) stm32vl.bin stm32vl.h

stm32lx.h: stm32lx.s
	$(CC) stm32lx.s $(CFLAGS_THUMB2) -o stm32lx.o
	$(OBJCOPY) -O binary stm32lx.o stm32lx.bin
	$(XXD) $(XXDFLAGS) stm32lx.bin stm32lx.h

stm32f4.h: stm32f4.s
	$(CC) stm32f4.s $(CFLAGS_THUMB2) -o stm32f4.o
	$(OBJCOPY) -O binary stm32f4.o stm32f4.bin
	$(XXD) $(XXDFLAGS) stm32f4.bin stm32f4.h

stm32f4lv.h: stm32f4lv.s
	$(CC) stm32f4lv.s $(CFLAGS_THUMB2) -o stm32f4lv.o
	$(OBJCOPY) -O binary stm32f4lv.o stm32f4lv.bin
	$(XXD) $(XXDFLAGS) stm32f4lv.bin stm32f4lv.h

stm32l4.h: stm32l4.s
	$(CC) stm32l4.s $(CFLAGS_THUMB2) -o stm32l4.o
	$(OBJCOPY) -O binary stm32l4.o stm32l4.bin
	$(XXD) $(XXDFLAGS) stm32l4.bin stm32l4.h

stm32f7.h: stm32f7.s
	$(CC) stm32f7.s $(CFLAGS_THUMB2) -o stm32f7.o
	$(OBJCOPY) -O binary stm32f7.o stm32f7.bin
	$(XXD) $(XXDFLAGS) stm32f7.bin stm32f7.h

stm32f7lv.h: stm32f7lv.s
	$(CC) stm32f7lv.s $(CFLAGS_THUMB2) -o stm32f7lv.o
	$(OBJCOPY) -O binary stm32f7lv.o stm32f7lv.bin
	$(XXD) $(XXDFLAGS) stm32f7lv.bin stm32f7lv.h

clean:
	rm *.o
	rm *.bin
	rm *.h
