% ST-UTIL(1) Open Source STMicroelectronics Stlink Tools  | STLINK
%
% Sep 2016


# NAME
st-util - Run GDB server to interact with STM32 device


# SYNOPSIS
*st-util* \[\<ARGS>...]


# DESCRIPTION
Start a GDB server to interact with a STM32 device
Run the main binary of the local package (src/main.rs).

If a port number is not specified using the **--listen_port** option, the
default **4242** port will be used.

Stlink version 2 is used by default unless the option **--stlinkv1** is given.

The STLinkV2 device to use can be specified in the environment
variable STLINK_DEVICE on the format <USB_BUS>:<USB_ADDR>.


# OPTIONS

-h, --help
:   Print this message.

--version
:   Print version information

-v *XX*, --verbose=XX
:   Specify a specific verbosity level (0..99)

-v, --verbose
:   Specify generally verbose logging

-s *X*, --stlink_version=X
:   Choose what version of stlink to use, (defaults to 2)

-1, --stlinkv1
:   Force stlink version 1

-p *4242*, --listen_port=1234
:   Set the gdb server listen port. (default port: 4242)

-m, --multi
:   Set gdb server to extended mode. st-util will continue listening for connections after disconnect.

-n, --no-reset
:   Do not reset board on connection.


# EXAMPLES
Run GDB server on port 4500 and connect to it

    $ st-util -p 4500
    $ gdb
    (gdb) target extended-remote localhost:4500


# SEE ALSO
st-flash(1), st-info(1)


# COPYRIGHT
This work is copyrighted. Stlink contributors.
See *LICENSE* file in the stlink source distribution.
