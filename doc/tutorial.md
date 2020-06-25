stlink Tools Tutorial
=====================

## Available tools and options

| Option | Tool | Description | Available<br />since |
| --- | --- | --- | --- |
| --flash=n[k][m] | st-flash | One can specify `--flash=128k` for example, to override the default value of 64k<br />for the STM32F103C8T6 to assume 128k of flash being present. This option accepts<br />decimal (128k), octal 0200k, or hex 0x80k values. Leaving the multiplier out is<br />equally valid, e.g.: `--flash=0x20000`. The size may be followed by an optional "k"<br />or "m" to multiply the given value by 1k (1024) or 1M respectively. | v1.4.0 |
| --freq=n[k][m] | st-flash,<br />st-util | The frequency of the SWD/JTAG interface can be specified, to override the default<br />1800 kHz configuration. This option solely accepts decimal values (5K or 1.8M) with<br />the unit `Hz` being left out. Valid frequencies are `5K, 15K, 25K, 50K, 100K,`<br />`125K, 240K, 480K, 950K, 1200K(1.2M), 1800K(1.8M), 4000K(4M)`. | v1.6.1 |
| --opt | st-flash | Optimisation can be enabled in order to skip flashing empty (0x00 or 0xff) bytes at<br />the end of binary file. This may cause some garbage data left after a flash operation.<br />This option was enabled by default in earlier releases. | v1.6.1 |
| --reset | st-flash | Trigger a reset both before and after flashing. | v1.0.0 |
| --version | st-info,<br />st-flash,<br />st-util | Print version information. | |
| --help | st-flash,<br />st-util | Print list of available commands. _(To be added to this table.)_ | |

### st-flash: Checksum for binary files
When flashing a file, a checksum is calculated for the binary file, both in md5 and the sum algorithm.
The latter is also used by the official ST-Link utility tool from STMicroelectronics as described in the document: [`UM0892 - User manual - STM32 ST-LINK utility software description`](https://www.st.com/resource/en/user_manual/cd00262073-stm32-stlink-utility-software-description-stmicroelectronics.pdf).

### stlink-gui
The `stlink` toolset also provides a GUI which is an optional feature. It is only installed if a gtk3 toolset has been detected during package installation or compilation from source. It is not available for Windows. If you prefer to have an user interface on the latter system, please use the official `ST-LINK Utility` instead.

The stlink-gui offers the following features:
* Connect/disconnect to a present STlink programmer
* Display basic device information
* Select a binary file from the local filesystem to flash it to the detected device connected to the programmer
* Export the memory of the connected chip to a file which can be saved to the local filesystem
* Display of the memory address map in the main window for each, the device memory and a loaded binary file

Within the GUI main window tooltips explain the available user elements.


## Solutions to common problems
### a) Verify if udev rules are set correctly (by Dave Hylands)

To investigate, start by plugging your STLINK device into the usb port. Then run `lsusb`. You should see an entry something like the following:

```
Bus 005 Device 017: ID 0483:374b STMicroelectronics ST-LINK/V2.1 (Nucleo-F103RB)
```

Note the bus number (005) and the Device (017). You should then do:
`ls -l /dev/bus/usb/005/017` (replacing 005 and 017 appropriately).

On my system I see the following:

```
crw-rw-rw- 1 root root 189, 528 Jan 24 17:52 /dev/bus/usb/005/017
```

which is world writable (this is from the `MODE:="0666"` below). I have several files in my `/etc/udev/rules.d` directory. In this particular case, the `49-stlinkv2-1.rules` file contains the following:

```
# stm32 nucleo boards, with onboard st/linkv2-1
# ie, STM32F0, STM32F4.
# STM32VL has st/linkv1, which is quite different

SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", \
    MODE:="0666", \
    SYMLINK+="stlinkv2-1_%n"

# If you share your linux system with other users, or just don't like the
# idea of write permission for everybody, you can replace MODE:="0666" with
# OWNER:="yourusername" to create the device owned by you, or with
# GROUP:="somegroupname" and mange access using standard unix groups.
```

and the `idVendor` of `0483` and `idProduct` of `374b` matches the vendor id from the `lsusb` output.

Make sure that you have all 3 files from here: https://github.com/stlink-org/stlink/tree/master/etc/udev/rules.d in your `/etc/udev/rules.d` directory. After copying new files or editing excisting files in `/etc/udev/ruled.d` you should run the following:

```
sudo udevadm control --reload-rules
sudo udevadm trigger
```

to ensure that the rules actually take effect. Using the trigger command means that you shouldn't need to unplug and replug the device, but you might want to try that for good measure as well.

If the VID:PID of your device doesn't match those in any of the 3 files, then you may need to create a custom rule file to match your VID:PID.


------
( Content below is currently unrevised and may be outdated as of Apr 2020. )

Using STM32 discovery kits with open source tools
========

This guide details the use of STMicroelectronics STM32 discovery kits in an open source environment.

## Installing a GNU toolchain

Any toolchain supporting the cortex m3 should do.

## Using the GDB server

This assumes you have got the [libopencm3](http://www.libopencm3.org) project downloaded in `ocm3`.
The libopencm3 project provides a firmware library, with solid examples for Cortex M3, M4 and M0 processors
from any vendor. It has some good, reliable examples for each of the Discovery boards.

Even if you don’t plan on using libopencm3, the examples they provide
will help you verify that:

-   Your installed toolchain is capable of compiling for cortex M3/M4
    targets
-   stlink is functional
-   Your arm-none-eabi-gdb is functional
-   Your board is functional

A GDB server must be started to interact with the STM32 by running
st-util tool :

```
$> st-util

# Full help for other options (listen port, version)
$> st-util --help
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


## Building and flashing a program

If you want to simply flash binary files to arbitrary sections of
memory, or read arbitary addresses of memory out to a binary file, use
the st-flash tool, as shown below:

```
# stlink command to read 4096 from flash into out.bin
$> ./st-flash read out.bin 0x8000000 4096

# stlinkv command to write the file in.bin into flash
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


## Using the gdb server

To run the gdb server:

```
$ make && [sudo] ./st-util

There are a few options:

./st-util - usage:

  -h, --help		Print this help
  -vXX, --verbose=XX	Specify a specific verbosity level (0..99)
  -v, --verbose		Specify generally verbose logging
  -p 4242, --listen_port=1234
			Set the gdb server listen port. (default port: 4242)
  -m, --multi
			Set gdb server to extended mode.
			st-util will continue listening for connections after disconnect.
  -n, --no-reset
			Do not reset board on connection.
```

The STLink device to use can be specified using the --serial parameter, or via
the environment variable STLINK_DEVICE on the format <USB_BUS>:<USB_ADDR>.

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


## Disassembling THUMB code in GDB

By default, the disassemble command in GDB operates in ARM mode. The programs running on CORTEX-M3 are compiled in THUMB mode.
To correctly disassemble them under GDB, uses an odd address. For instance, if you want to disassemble the code at 0x20000000, use:

```
(gdb) disassemble 0x20000001
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
