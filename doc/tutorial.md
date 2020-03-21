Using STM32 discovery kits with open source tools
========

This guide details the use of STMicroelectronics STM32 discovery kits in an open source environment.

Installing a GNU toolchain
==========================

Any toolchain supporting the cortex m3 should do. You can find the
necessary to install such a toolchain here:

```
https://github.com/esden/summon-arm-toolchain
```

Details for the installation are provided in the topmost `README` file.
This documentation assumes the toolchains is installed in a
`$TOOLCHAIN_PATH`.

Installing STLINK
=================

STLINK is open source software to program and debug ST’s STM32 Discovery
kits. Those kits have an onboard chip that translates USB commands sent
by the host PC into JTAG/SWD commands. This chip is called STLINK, (yes,
isn’t that confusing? suggest a better name!) and comes in 2 versions
(STLINK v1 and v2). From a software point of view, those versions differ
only in the transport layer used to communicate (v1 uses SCSI passthru
commands, while v2 uses raw USB). From a user point of view, they are
identical.


Before continuing, the following dependencies must be met:

-   libusb-1.0
-   cmake

Also make sure `gtk+` version 3.0 is installed. (`sudo apt-get install gtk+-3.0` or similiar)

STLINK should run on any system meeting the above constraints.

The STLINK software source code is retrieved using:

```
$> git clone https://github.com/texane/stlink.git
```

Everything can be built from the top directory:

```
$> cd stlink
$> make
$> cd build/Release && make install DESTDIR=_install
```

It includes:

- a communication library (libstlink.a),
- a GDB server (st-util),
- a flash manipulation tool (st-flash).
- a programmer and chip information tool (st-info)

Using the GDB server
====================

This assumes you have got the libopencm3 project downloaded in `ocm3`.
The libopencm3 project has some good, reliable examples for each of the
Discovery boards.

Even if you don’t plan on using libopencm3, the examples they provide
will help you verify that:

-   Your installed toolchain is capable of compiling for cortex M3/M4
    targets
-   stlink is functional
-   Your arm-none-eabi-gdb is functional
-   Your board is functional

A GDB server must be started to interact with the STM32. Depending on
the discovery kit you are using, you must run one of the 2 commands:

```
# STM32VL discovery kit (onboard ST-link)
$> ./st-util --stlinkv1

# STM32L or STM32F4 discovery kit (onboard ST-link/V2)
$> ./st-util

# Full help for other options (listen port, version)
$> ./st-util --help
```

Then, GDB can be used to interact with the kit:

```
$> $TOOLCHAIN_PATH/bin/arm-none-eabi-gdb example_file.elf
```

From GDB, connect to the server using:

```
(gdb) target extended localhost:4242
```

GDB has memory maps for as many chips as it knows about, and will load
your project into either flash or SRAM based on how the project was
linked. Linking projects to boot from SRAM is beyond the scope of this
document.

Because of these built in memory maps, after specifying the .elf at the
command line, now we can simply “load” the target:

```
(gdb) load
```

st-util will load all sections into their appropriate addresses, and
“correctly” set the PC register. So, to run your freshly loaded program,
simply “continue”

```
(gdb) continue
```

Your program should now be running, and, if you used one of the blinking
examples from libopencm3, the LEDs on the board should be blinking for
you.


Building and flashing a program
===============================

If you want to simply flash binary files to arbitrary sections of
memory, or read arbitary addresses of memory out to a binary file, use
the st-flash tool, as shown below:

```
# stlinkv1 command to read 4096 from flash into out.bin
$> ./st-flash read out.bin 0x8000000 4096

# stlinkv2 command
$> ./st-flash read out.bin 0x8000000 4096

# stlinkv1 command to write the file in.bin into flash
$> ./st-flash write in.bin 0x8000000

# stlinkv2 command
$> ./st-flash write in.bin 0x8000000
```

It is also possible to write a hexfile which is more convinient:

```
$> ./st-flash --format ihex write myapp.hex
```

####

Of course, you can use this instead of the gdb server, if you prefer.
Just remember to use the “.bin” image, rather than the .elf file.

```

# write blink.bin into FLASH
$> [sudo] ./st-flash write fancy_blink.bin 0x08000000
```

Upon reset, the board LEDs should be blinking.


HOWTO
=====
## Using the gdb server

To run the gdb server:

```
$ make && [sudo] ./st-util

There are a few options:

./st-util - usage:

  -h, --help		Print this help
  -vXX, --verbose=XX	Specify a specific verbosity level (0..99)
  -v, --verbose		Specify generally verbose logging
  -s X, --stlink_version=X
			Choose what version of stlink to use, (defaults to 2)
  -1, --stlinkv1	Force stlink version 1
  -p 4242, --listen_port=1234
			Set the gdb server listen port. (default port: 4242)
  -m, --multi
			Set gdb server to extended mode.
			st-util will continue listening for connections after disconnect.
  -n, --no-reset
			Do not reset board on connection.
```

The STLINKv2 device to use can be specified in the environment
variable `STLINK_DEVICE` in the format `<USB_BUS>:<USB_ADDR>`.

Then, in your project directory, someting like this...
(remember, you need to run an _ARM_ gdb, not an x86 gdb)

```
$ arm-none-eabi-gdb fancyblink.elf
...
(gdb) tar extended-remote :4242
...
(gdb) load
Loading section .text, size 0x458 lma 0x8000000
Loading section .data, size 0x8 lma 0x8000458
Start address 0x80001c1, load size 1120
Transfer rate: 1 KB/sec, 560 bytes/write.
(gdb)
...
(gdb) continue
```


## Resetting the chip from GDB

You may reset the chip using GDB if you want. You'll need to use `target
extended-remote' command like in this session:

```
(gdb) target extended-remote localhost:4242
Remote debugging using localhost:4242
0x080007a8 in _startup ()
(gdb) kill
Kill the program being debugged? (y or n) y
(gdb) run
Starting program: /home/whitequark/ST/apps/bally/firmware.elf
```

Remember that you can shorten the commands. `tar ext :4242` is good enough
for GDB.

If you need to send a hard reset signal through `NRST` pin, you can use the following command:

```
(gdb) monitor jtag_reset
```


## Running programs from SRAM

You can run your firmware directly from SRAM if you want to. Just link
it at 0x20000000 and do

```
(gdb) load firmware.elf
```

It will be loaded, and pc will be adjusted to point to start of the
code, if it is linked correctly (i.e. ELF has correct entry point).


## Writing to flash

The GDB stub ships with a correct memory map, including the flash area.
If you would link your executable to `0x08000000` and then do

```
(gdb) load firmware.elf
```

then it would be written to the memory.


## Writing Option Bytes

Example to read and write option bytes (currently writing only supported for STM32G0 and STM32L0)
```
./st-flash --debug --reset --format binary --flash=128k read option_bytes_dump.bin 0x1FFF7800 4
./st-flash --debug --reset --format binary --flash=128k write option_bytes_dump.bin 0x1FFF7800
```


FAQ
===

Q: My breakpoints do not work at all or only work once.

A: Optimizations can cause severe instruction reordering. For example, if you are doing something like `REG = 0x100;' in a loop, the code may be split into two parts: loading 0x100 into some intermediate register and moving that value to REG. When you set up a breakpoint, GDB will hook to the first instruction, which may be called only once if there are enough unused registers. In my experience, -O3 causes that frequently.

Q: At some point I use GDB command `next', and it hangs.

A: Sometimes when you will try to use GDB `next` command to skip a loop, it will use a rather inefficient single-stepping way of doing that. Set up a breakpoint manually in that case and do `continue`.

Q: Load command does not work in GDB.

A: Some people report XML/EXPAT is not enabled by default when compiling GDB. Memory map parsing thus fail. Use --enable-expat.


Notes
=====

Disassembling THUMB code in GDB
-------------------------------

By default, the disassemble command in GDB operates in ARM mode. The
programs running on CORTEX-M3 are compiled in THUMB mode. To correctly
disassemble them under GDB, uses an odd address. For instance, if you
want to disassemble the code at 0x20000000, use:\

```
(gdb) disassemble 0x20000001
```

References
==========

-   <http://www.st.com/internet/mcu/product/248823.jsp>
    documentation related to the STM32L mcu

-   <http://www.st.com/internet/evalboard/product/250990.jsp>
    documentation related to the STM32L discovery kit

-   <http://www.libopencm3.org>
    libopencm3, a project providing a firmware library, with solid
    examples for Cortex M3, M4 and M0 processors from any vendor.
