% ST-INFO(1) Open Source STMicroelectronics Stlink Tools  | STLINK
%
% Sep 2016


# NAME
st-info - Provides information about connected STLink and STM32 devices


# SYNOPSIS
*st-info* \[*OPTIONS*\]


# DESCRIPTION
Provides information about connected STLink programmers and STM32 devices:
Serial code, openocd, flash, sram, page size, chipid, description.


# OPTIONS

--version
:   Print version information

--flash
:   Display amount of flash memory available in the device

--sram
:   Display amount of sram memory available in device

--descr
:   Display textual description of the device

--pagesize
:   Display the page size of the device

--chipid
:   Display the chip ID of the device

--serial
:   Display the serial code of the device

--hla-serial
:   Display the hex escaped serial code of the device

--probe
:   Display the summarized information of the connected programmers and devices


# EXAMPLES
Display information about connected programmers and devices

    $ st-info --probe


# SEE ALSO
st-util(1), st-flash(1)


# COPYRIGHT
This work is copyrighted. Stlink contributors.
See *LICENSE* file in the stlink source distribution.
