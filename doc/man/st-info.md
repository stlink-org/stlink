% st-flash(1) Open Source STMicroelectronics Stlink Tools  | stlink
%
% Oct 2020

# NAME
st-info - Provides information about connected STLink and STM32 devices


# SYNOPSIS
*st-info* \[*OPTIONS*\]


# DESCRIPTION

Provides information about connected STLink programmers and STM32 devices:
Serial code, OpenOCD hla-serial, flash, page size, sram, chipid, description.

The STLink device to probe can be specified via the environment variable
STLINK_DEVICE on the format <USB_BUS>:<USB_ADDR>.

# OPTIONS

\--version
:   Print version information

\--probe
:   Display the summarized information of the connected programmers and devices

\--serial
:   Display the serial code of the device

\--hla-serial
:   Display the hex escaped serial code of the device

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
     serial:     303033413030323233343338353130323334333133393339
     hla-serial: "\x30\x30\x33\x41\x30\x30\x32\x32\x33\x34\x33\x38\x35\x31\x30\x32\x33\x34\x33\x31\x33\x39\x33\x39"
     flash:      131072 (pagesize: 128)
     sram:       20480
     chipid:     0x0447
     descr:      L0xx Category 5


# SEE ALSO
st-util(1), st-flash(1)


# COPYRIGHT
This work is copyrighted. Stlink contributors.
See *LICENSE* file in the stlink source distribution.
