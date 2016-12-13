Open source version of the STMicroelectronics Stlink Tools
==========================================================

[![GitHub release](https://img.shields.io/github/release/texane/stlink.svg)](https://github.com/texane/stlink/releases/latest)
[![GitHub commits](https://img.shields.io/github/commits-since/texane/stlink/1.2.0.svg)](https://github.com/texane/stlink/compare/1.2.0...master)
[![Linux Status](https://img.shields.io/travis/texane/stlink/master.svg?label=linux)](https://travis-ci.org/texane/stlink)
[![Build Status](https://jenkins.ncrmnt.org/buildStatus/icon?job=GithubCI/stlink)](https://jenkins.ncrmnt.org/job/GithubCI/job/stlink/)
[![Build status](https://ci.appveyor.com/api/projects/status/wrcie05d4jmut0te?svg=true)](https://ci.appveyor.com/project/xor-gate/stlink)
[![BSD licensed](https://img.shields.io/badge/license-BSD-blue.svg)](https://raw.githubusercontent.com/hyperium/hyper/master/LICENSE)

## HOWTO

First, you have to know there are several boards supported by the software.
Those boards use a chip to translate from USB to JTAG commands. The chip is
called stlink and there are two versions:

* STLINKv1, present on STM32VL discovery kits,
* STLINKv2, present on STM32L discovery and later kits.

Two different transport layers are used:

* STLINKv1 uses SCSI passthru commands over USB
* STLINKv2 uses raw USB commands.

## Installation

Currently there are no binaries for Windows available.
 It is known to compile and work with MinGW/Cygwin.

For Debian Linux based distributions there is also no package available
 in the standard repositories so you need to compile yourself.

Arch Linux users can install from the [repository](https://www.archlinux.org/packages/community/x86_64/stlink)

FreeBSD users can install from [freshports](https://www.freshports.org/devel/stlink)

Mac OS X users can install from [homebrew](http://brewformulas.org/Stlink)

**Compilation from source (advanced users)**

When there is no executable available for your platform or you need the latest
 (possible unstable) version you need to compile yourself. This is explained in
 the [compiling manual](doc/compiling.md).

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

Remember that you can shorten the commands. `tar ext :4242' is good enough
for GDB.

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

## FAQ

Q: My breakpoints do not work at all or only work once.

A: Optimizations can cause severe instruction reordering. For example,
if you are doing something like `REG = 0x100;' in a loop, the code may
be split into two parts: loading 0x100 into some intermediate register
and moving that value to REG. When you set up a breakpoint, GDB will
hook to the first instruction, which may be called only once if there are
enough unused registers. In my experience, -O3 causes that frequently.

Q: At some point I use GDB command `next', and it hangs.

A: Sometimes when you will try to use GDB `next' command to skip a loop,
it will use a rather inefficient single-stepping way of doing that.
Set up a breakpoint manually in that case and do `continue'.

Q: Load command does not work in GDB.

A: Some people report XML/EXPAT is not enabled by default when compiling
GDB. Memory map parsing thus fail. Use --enable-expat.

## Currently known working combinations of programmer and target

STLink v1 (as found on the 32VL Discovery board)

Known working targets:

* STM32F100xx (Medium Density VL)
* STM32F103 (according to jpa- on ##stm32)

No information:

* everything else!

STLink v2 (as found on the 32L and F4 Discovery boards), known working targets:

* STM32F030F4P6 (custom board)
* STM32F0Discovery (STM32F0 Discovery board)
* STM32F100xx (Medium Density VL, as on the 32VL Discovery board)
* STM32L1xx (STM32L Discovery board)
* STM32F103VC, STM32F107RC, STM32L151RB, STM32F205RE and STM32F405RE on custom boards
  from [UweBonnes/wiki_fuer_alex](https://github.com/UweBonnes/wiki_fuer_alex/tree/master/layout)
* STM32F103VET6 (HY-STM32 board)
* STM32F105RCT6 (DecaWave EVB1000 board)
* STM32F303xx (STM32F3 Discovery board)
* STM32F407xx (STM32F4 Discovery board)
* STM32F429I-DISCO (STM32F4 Discovery board with LCD)
* STM32F439VIT6 (discovery board reseated CPU)
* STM32L052K8T6 (custom board)
* STM32L151CB (custom board)
* STM32L152RB (STM32L-Discovery board, custom board)
* STM32F051R8T6 (STM320518-EVAL board)
* STM32F411E-DISCO (STM32F4 Discovery board with gyro, audio)

STLink v2-1 (as found on the Nucleo boards), known working targets:

* STM32F401xx (STM32 Nucleo-F401RE board) 
* STM32F030R8T6 (STM32 Nucleo-F030R8 board)
* STM32F072RBT6 (STM32 Nucleo-F072RB board)
* STM32F103RB (STM32 Nucleo-F103RB board)
* STM32F303RET6 (STM32 Nucleo-F303RE board)
* STM32F334R8 (STM32 Nucleo-F334R8 board)
* STM32F411RET6 (STM32 Nucleo-F411RE board)
* STM32F756NGHx (STMF7 evaluation board)
* STM32L053R8 (STM32 Nucleo-L053R8 board)
* STM32F769NI (STM32F7 discovery board)

Please report any and all known working combinations so I can update this!

## Missing features

* Control programming speed (See [#462](https://github.com/texane/stlink/issues/462))
* OTP area programming (See [#202](https://github.com/texane/stlink/issues/202))
* EEPROM area programming (See [#318](https://github.com/texane/stlink/issues/218))
* Protection bits area reading (See [#346](https://github.com/texane/stlink/issues/346))

## Known bugs

### Sometimes flashing only works after a mass erase

There is seen a problem sometimes where a flash loader run error occurs and is resolved after mass-erase
of the flash:

```
2015-12-09T22:01:57 INFO src/stlink-common.c: Successfully loaded flash loader in sram
2015-12-09T22:02:18 ERROR src/stlink-common.c: flash loader run error
2015-12-09T22:02:18 ERROR src/stlink-common.c: run_flash_loader(0x8000000) failed! == -1
```

Issue(s): [#356](https://github.com/texane/stlink/issues/356)

## Contributing and versioning

* The semantic versioning scheme is used. Read more at [semver.org](http://semver.org)
* When creating a pull request, please open first a issue for discussion of new features
* TODO: Enforcement of coding style (linux codestyle + checkpatch)

## License

The stlink library and tools are licensed under the [BSD license](LICENSE). With
some exceptions on external components.
