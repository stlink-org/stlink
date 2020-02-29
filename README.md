Open source version of the STMicroelectronics Stlink Tools
==========================================================

[![BSD licensed](https://img.shields.io/badge/license-BSD-blue.svg)](https://raw.githubusercontent.com/hyperium/hyper/master/LICENSE)
[![GitHub release](https://img.shields.io/github/release/texane/stlink.svg)](https://github.com/texane/stlink/releases/latest)
[![GitHub commits](https://img.shields.io/github/commits-since/texane/stlink/v1.6.0.svg)](https://github.com/texane/stlink/releases/master)
[![Downloads](https://img.shields.io/github/downloads/texane/stlink/total.svg)](https://github.com/texane/stlink/releases)
[![Linux Status](https://img.shields.io/travis/texane/stlink/master.svg?label=linux)](https://travis-ci.org/texane/stlink)
[![macOS Status](https://img.shields.io/travis/texane/stlink/master.svg?label=osx)](https://travis-ci.org/texane/stlink)

Recent new features and bugfixes can be found in the [Changelog](CHANGELOG.md) of this software project.


## Introduction

This stlink toolset supports several so called stlink programmer boards (and clones thereof) which use a microcontroller chip to translate commands from USB to JTAG.

These programmer boards are available in four versions:

* **STLINKv1:**
  - transport layer: SCSI passthru commands over USB
  - present on STM32VL discovery kits
* **STLINKv2:**
  * transport layer: raw USB commands
  * present on STM32L discovery and nucleo and later kits
* **STLINKv2-1:**
  * transport layer: raw USB commands
  * present on some STM32 nucleo boards
* **STLINKv3:**
  * _not yet supported by this toolset (but planned)_


## Supported hardware combinations

Currently known working combinations of programmers and targets are listed in [doc/tested-boards.md](doc/tested-boards.md).


## Installation

**Windows**: download [v1.6.0](https://github.com/texane/stlink/releases/tag/v1.6.0) from the releases page.


**macOS**: install [from homebrew](http://brewformulas.org/Stlink) or download [v1.6.0](https://github.com/texane/stlink/releases/tag/v1.6.0) from the releases page.

**Linux**:

We recommend to install `stlink-tools` from the package repository of the used distribution:

* Debian Linux: [(Link)](https://packages.debian.org/buster/stlink-tools)
* Ubuntu Linux: [(Link)](https://packages.ubuntu.com/stlink-tools)
* Arch Linux:   [(Link)](https://www.archlinux.org/packages/community/x86_64/stlink)
* Alpine Linux: [(Link)](https://pkgs.alpinelinux.org/packages?name=stlink)
* Fedora:       [(Link)](https://src.fedoraproject.org/rpms/stlink)
* Gentoo Linux: [(Link)](https://packages.gentoo.org/packages/dev-embedded/stlink)

**Other Operating Systems**:

* RedHat/CentOS 7: Users can install [from EPEL repository](https://src.fedoraproject.org/rpms/stlink/branch/epel7)
* FreeBSD: Users can install [from freshports](https://www.freshports.org/devel/stlink)
* OpenBSD: Users need to install [from source](doc/compiling.md).


## Installation from source (advanced users)

When there is no executable available for your platform or you need the latest (possible unstable) version you need to compile the toolset yourself. This procedure is explained in the [compiling manual](doc/compiling.md).


## Contributing and versioning

* The semantic versioning scheme is used. Read more at [semver.org](http://semver.org)
* Before creating a pull request, please _ALWAYS_ open a new issue for the discussion of the intended new features. Bugfixes don't require a discussion via a ticket-issue. However they should always be described in a few words as soon as they appear to help others as well.
* Contributors and/or maintainers may submit comments or request changes to patch-proposals and/or pull-requests.
* **ATTENTION: _NEVER EVER_ use the '#' character to count-up single points within a listing as '#' is _exclusively_ reserved for referencing github issues and pull-requests. Otherwise you accidentally introduce false cross references within the project.**
* Please start new forks from the develop branch if possible as pull requests will go into this branch as well.


## License

The stlink library and tools are licensed under the [BSD license](LICENSE.md).

The flashloaders/stm32l0x.s and flashloaders/stm32lx.s source files are licensed under the GPLv2+.


# Current state of the project
## Known missing features

Some features are currently missing from the `texane/stlink` toolset.
Here we would appreciate any help and would love to welcome new contributors who want to get involved:

* Instrumentation Trace Macro (ITM) Cell ([#136](https://github.com/texane/stlink/issues/136))
* OTP area programming ([#202](https://github.com/texane/stlink/issues/202))
* EEPROM area programming ([#318](https://github.com/texane/stlink/issues/218))
* Protection bits area reading ([#346](https://github.com/texane/stlink/issues/346))
* Writing external memory connected to an STM32 controller (e.g Quad SPI NOR flash) ([#412](https://github.com/texane/stlink/issues/412))
* MCU hotplug ([#449](https://github.com/texane/stlink/issues/449))
* Writing options bytes (region) ([#458](https://github.com/texane/stlink/issues/458))
* Control programming speed ([#462](https://github.com/texane/stlink/issues/462))
* Support for STLINKv3 programmer ([#820](https://github.com/texane/stlink/issues/820))


## Known bugs
### Sometimes flashing only works after a mass erase

There is seen a problem sometimes where a flash loader run error occurs and is resolved after mass-erase of the flash:

```
2015-12-09T22:01:57 INFO src/stlink-common.c: Successfully loaded flash loader in sram
2015-12-09T22:02:18 ERROR src/stlink-common.c: flash loader run error
2015-12-09T22:02:18 ERROR src/stlink-common.c: run_flash_loader(0x8000000) failed! == -1
```

Issue related to this bug: [#356](https://github.com/texane/stlink/issues/356)


### Flash size is detected as zero bytes size

It is possible that the STM32 flash is write protected, the st-flash tool will show something like this:

```
st-flash write prog.bin 0x8000000
2017-01-24T18:44:14 INFO src/stlink-common.c: Loading device parameters....
2017-01-24T18:44:14 INFO src/stlink-common.c: Device connected is: F1 High-density device, id 0x10036414
2017-01-24T18:44:14 INFO src/stlink-common.c: SRAM size: 0x10000 bytes (64 KiB), Flash: 0 bytes (0 KiB) in pages of 2048 bytes
```

As you can see, it gives out something unexpected like
```
Flash: 0 bytes (0 KiB) in pages of 2048 bytes
```

```
st-info --probe
Found 1 stlink programmers
 serial: 303030303030303030303031
openocd: "\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x31"
  flash: 0 (pagesize: 2048)
   sram: 65536
 chipid: 0x0414
  descr: F1 High-density device
```

Try to remove the write protection (probably only possible with ST Link Utility from ST itself).

Issue related to this bug: [#545](https://github.com/texane/stlink/issues/545)


# HOWTO
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


## FAQ

Q: My breakpoints do not work at all or only work once.

A: Optimizations can cause severe instruction reordering. For example, if you are doing something like `REG = 0x100;' in a loop, the code may be split into two parts: loading 0x100 into some intermediate register and moving that value to REG. When you set up a breakpoint, GDB will hook to the first instruction, which may be called only once if there are enough unused registers. In my experience, -O3 causes that frequently.

Q: At some point I use GDB command `next', and it hangs.

A: Sometimes when you will try to use GDB `next` command to skip a loop, it will use a rather inefficient single-stepping way of doing that. Set up a breakpoint manually in that case and do `continue`.

Q: Load command does not work in GDB.

A: Some people report XML/EXPAT is not enabled by default when compiling GDB. Memory map parsing thus fail. Use --enable-expat.
