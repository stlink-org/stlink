% st-flash(1) Open Source STMicroelectronics Stlink Tools  | stlink
%
% Feb 2018

# NAME

st-flash - Flash binary files to STM32 device

# SYNOPSIS

*st-flash* \[*OPTIONS*\] \{read|write|erase\} \[*FILE*\] \<ADDR\> \<SIZE\>

# DESCRIPTION

Flash binary files to arbitrary sections of memory, or read arbitrary addresses
of memory out to a binary file.

You can use this instead of st-util(1) if you prefer, but remember to use the
**.bin** image, rather than the **.elf** file.

Use hexadecimal format for the *ADDR* and *SIZE*.

The STLink device to use can be specified using the --serial parameter, or via 
the environment variable STLINK_DEVICE on the format <USB_BUS>:<USB_ADDR>.

# COMMANDS

write *FILE* *ADDR*
:   Write firmware *FILE* to device starting from *ADDR*

read *FILE* *ADDR* *SIZE*
:   Read firmware from device starting from *ADDR* up to *SIZE* bytes to *FILE*

erase
:   Perform a mass erasing of the device firmware

reset
:   Reset the target

# OPTIONS

\--version
:   Print version information

\--debug
:   TODO

\--reset
:   TODO

\--opt
:   Enable ignore ending empty bytes optimization

\--serial *iSerial*
:   TODO

\--flash=fsize
:   Where fsize is the size in decimal, octal, or hex followed by an optional multiplier 
'k' for KB, or 'm' for MB.
Use a leading "0x" to specify hexadecimal, or a leading zero for octal.

# EXAMPLES

Flash `firmware.bin` to device

    $ st-flash write firmware.bin 0x8000000

Read firmware from device (4096 bytes)

    $ st-flash read firmware.bin 0x8000000 0x1000

Erase firmware from device

    $ st-flash erase

# SEE ALSO

st-util(1), st-info(1)

# COPYRIGHT

This work is copyrighted. Stlink contributors.
See *LICENSE* file in the stlink source distribution.
