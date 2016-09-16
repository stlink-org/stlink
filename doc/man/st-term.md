% ST-TERM(1) Open Source STMicroelectronics Stlink Tools  | STLINK
%
% Sep 2016


# NAME
st-term - Serial terminal interface for debugging using STLink device only


# SYNOPSIS
*st-term*


# DESCRIPTION
Provides a serial terminal that works through the STLink device, allowing to do
the kind of debugging you would do using the UART of the STM32 device without
having to connect a UART-USB serial adapter dongle.

It works by having a magic number in sram of the MCU within a structure that
serves as IO buffer.

The required setup consist of some code that establishes the magic number,
buffer structure and helper functions for actual data transmission between host
and MCU.

`stlinky.h` and `stlinky.c` are available in the Antares build system libraries
source: https://github.com/nekromant/antares/tree/master/include/lib


# SEE ALSO
st-util(1), st-info(1), st-term(1)


# COPYRIGHT
This work is copyrighted. Stlink contributors.
See *LICENSE* file in the stlink source distribution.
