% st-flash(1) Open Source STMicroelectronics Stlink Tools  | stlink
%
% Oct 2020

# NAME
st-info - Provides information about connected STLink and STM32 devices


# SYNOPSIS
*st-info* \[*OPTIONS*\]


# DESCRIPTION

Provides information about connected STLink programmers and STM32 devices:
Serial code, flash, page size, sram, chipid, description.

# OPTIONS

\--version
:   Print version information

\--probe
:   Display the summarized information of the connected programmers and devices

\--serial
:   Display the serial code of the device

\--flash
:   Display amount of flash memory available in the device

\--pagesize
:   Display the page size of the device

\--sram
:   Display amount of sram memory available in device

\--chipid
:   Display the chip ID of the device

\--descr
:   Display textual description of the device


# EXAMPLES
Display information about connected programmers and devices

    $ st-info --probe
    Found 1 stlink programmers
     serial:     57FF72067265575742132067
     flash:      131072 (pagesize: 128)
     sram:       20480
     chipid:     0x0447
     descr:      L0xx Category 5


# SEE ALSO
st-util(1), st-flash(1)


# COPYRIGHT
This work is copyrighted. Stlink contributors.
See *LICENSE* file in the stlink source distribution.
