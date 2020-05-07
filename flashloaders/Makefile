# Note that according to the original GPLed code, compiling is noted to be
# as simple as gcc -c, this fails with my tests where this will lead to a wrong 
# address read by the program.
# This makefile will save your time from dealing with compile errors
# Adjust CC if needed

CC = /opt/local/gcc-arm-none-eabi-8-2018-q4-major/bin/arm-none-eabi-gcc

CFLAGS_thumb1 = -mcpu=Cortex-M0  -Tlinker.ld -ffreestanding -nostdlib
CFLAGS_thumb2 = -mcpu=Cortex-M3  -Tlinker.ld -ffreestanding -nostdlib

all: stm32vl.o stm32f0.o stm32l.o stm32f4.o stm32f4_lv.o stm32l4.o stm32f7.o stm32f7_lv.o

stm32vl.o: stm32f0.s
	$(CC) stm32f0.s $(CFLAGS_thumb2)  -o stm32vl.o
stm32f0.o: stm32f0.s
	$(CC) stm32f0.s $(CFLAGS_thumb1)  -o stm32f0.o
stm32l.o: stm32lx.s
	$(CC) stm32lx.s $(CFLAGS_thumb2)  -o stm32l.o
stm32f4.o: stm32f4.s
	$(CC) stm32f4.s $(CFLAGS_thumb2)  -o stm32f4.o
stm32f4_lv.o: stm32f4lv.s
	$(CC) stm32f4lv.s $(CFLAGS_thumb2)  -o stm32f4_lv.o
stm32l4.o: stm32l4.s
	$(CC) stm32l4.s $(CFLAGS_thumb2)  -o stm32l4.o
stm32f7.o: stm32f7.s
	$(CC) stm32f7.s $(CFLAGS_thumb2)  -o stm32f7.o
stm32f7_lv.o: stm32f7lv.s
	$(CC) stm32f7lv.s $(CFLAGS_thumb2)  -o stm32f7_lv.o

clean:
	rm *.o






