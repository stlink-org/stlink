EXECUTABLE=STM32F4-Discovery_Demo.elf
BIN_IMAGE=STM32F4-Discovery_Demo.bin

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-g -O2 -mlittle-endian -mthumb
CFLAGS+=-mcpu=cortex-m4
CFLAGS+=-ffreestanding -nostdlib

#usb_conf.h
CFLAGS+=-DUSE_USB_OTG_FS=1

# to run from FLASH
CFLAGS+=-Wl,-T,stm32_flash.ld

CFLAGS+=-I./

# stm32f4_discovery lib
CFLAGS+=-I../../STM32F4xx_StdPeriph_Driver/inc
CFLAGS+=-I../../STM32F4xx_StdPeriph_Driver/inc/device_support
CFLAGS+=-I../../STM32F4xx_StdPeriph_Driver/inc/core_support

#STM32_USB_Device_Library
CFLAGS+=-I../../STM32_USB_Device_Library/Class/hid/inc
CFLAGS+=-I../../STM32_USB_Device_Library/Core/inc

#STM32_USB_OTG_Driver
CFLAGS+=-I../../STM32_USB_OTG_Driver/inc

#STM32F4xx_StdPeriph_Driver\inc
CFLAGS+=-I../../STM32F4xx_StdPeriph_Driver/inc

#Utilities
CFLAGS+=-I../../Utilities/STM32F4-Discovery

all: $(BIN_IMAGE)

$(BIN_IMAGE): $(EXECUTABLE)
	$(OBJCOPY) -O binary $^ $@

$(EXECUTABLE): main.c selftest.c system_stm32f4xx.c startup_stm32f4xx.s stm32f4xx_it.c \
	usb_bsp.c usbd_desc.c usbd_usr.c usb_core.c \
	../../Utilities/STM32F4-Discovery/stm32f4_discovery.c \
	../../Utilities/STM32F4-Discovery/stm32f4_discovery_audio_codec.c \
	../../Utilities/STM32F4-Discovery/stm32f4_discovery_lis302dl.c \
	../../STM32_USB_OTG_Driver/src/usb_dcd_int.c \
	../../STM32_USB_OTG_Driver/src/usb_dcd.c \
	../../STM32_USB_Device_Library/Core/src/usbd_core.c \
	../../STM32_USB_Device_Library/Core/src/usbd_req.c \
	../../STM32_USB_Device_Library/Core/src/usbd_ioreq.c \
	../../STM32_USB_Device_Library/Class/hid/src/usbd_hid_core.c \

		
	$(CC) $(CFLAGS) $^ -o $@  -L../../STM32F4xx_StdPeriph_Driver/build -lSTM32F4xx_StdPeriph_Driver -L../../STM32F_USB_OTG_Driver/build

clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(BIN_IMAGE)

.PHONY: all clean
