Open source version of the STMicroelectronics Stlink Tools
==========================================================

[![GitHub release](https://img.shields.io/github/release/texane/stlink.svg)](https://github.com/texane/stlink/releases/latest)
[![BSD licensed](https://img.shields.io/badge/license-BSD-blue.svg)](https://raw.githubusercontent.com/hyperium/hyper/master/LICENSE)
[![GitHub commits](https://img.shields.io/github/commits-since/texane/stlink/1.4.0.svg)](https://github.com/texane/stlink/compare/1.4.0...master)
[![Downloads](https://img.shields.io/github/downloads/texane/stlink/total.svg)](https://github.com/texane/stlink/releases)
[![Linux Status](https://img.shields.io/travis/texane/stlink/master.svg?label=linux)](https://travis-ci.org/texane/stlink)
[![Build Status](https://jenkins.ncrmnt.org/buildStatus/icon?job=GithubCI/stlink)](https://jenkins.ncrmnt.org/job/GithubCI/job/stlink/)

## HOWTO

First, you have to know there are several boards supported by the software.
Those boards use a chip to translate from USB to JTAG commands. The chip is
called stlink and there are two versions:

* STLINKv1, present on STM32VL discovery kits,
* STLINKv2, present on STM32L discovery and later kits.

Two different transport layers are used:

* STLINKv1 uses SCSI passthru commands over USB
* STLINKv2 and STLINKv2-1 (seen on nucleo boards) uses raw USB commands.

## Installation

Windows users can [download v1.3.0](https://github.com/texane/stlink/releases/tag/1.3.0) from the releases page.

Mac OS X users can install from [homebrew](http://brewformulas.org/Stlink) or [download v1.3.0](https://github.com/texane/stlink/releases/tag/1.3.0) from the releases page.

For Debian Linux based distributions there is no package available
 in the standard repositories so you need to install [from source](doc/compiling.md) yourself.

Arch Linux users can install from the [repository](https://www.archlinux.org/packages/community/x86_64/stlink)

Alpine Linux users can install from the [repository](https://pkgs.alpinelinux.org/packages?name=stlink)

Fedora users can install from [repository](https://src.fedoraproject.org/rpms/stlink)

RedHat/CentOS 7 users can install from EPEL [repository](https://src.fedoraproject.org/rpms/stlink/branch/epel7)

Gentoo Linux users can install from the official portage [repository](https://packages.gentoo.org/packages/dev-embedded/stlink)

FreeBSD users can install from [freshports](https://www.freshports.org/devel/stlink)

OpenBSD users need to install [from source](doc/compiling.md).

## Installation from source (advanced users)

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

A: Sometimes when you will try to use GDB `next` command to skip a loop,
it will use a rather inefficient single-stepping way of doing that.
Set up a breakpoint manually in that case and do `continue`.

Q: Load command does not work in GDB.

A: Some people report XML/EXPAT is not enabled by default when compiling
GDB. Memory map parsing thus fail. Use --enable-expat.

## Currently known working combinations of programmer and target

See [doc/tested-boards.md](doc/tested-boards.md)

## Known missing features

Some features are missing from the `texane/stlink` project and we would like you to
 help us out if you want to get involved:

* Control programming speed (See [#462](https://github.com/texane/stlink/issues/462))
* OTP area programming (See [#202](https://github.com/texane/stlink/issues/202))
* EEPROM area programming (See [#318](https://github.com/texane/stlink/issues/218))
* Protection bits area reading (See [#346](https://github.com/texane/stlink/issues/346))
* MCU hotplug (See [#449](https://github.com/texane/stlink/issues/449))
* Writing options bytes (region) (See [#458](https://github.com/texane/stlink/issues/458))
* Instrumentation Trace Macro (ITM) Cell (See [#136](https://github.com/texane/stlink/issues/136))
* Writing external memory connected to an STM32 controller (e.g Quad SPI NOR flash) (See [#412](https://github.com/texane/stlink/issues/412))

## Known bugs

### Sometimes flashing only works after a mass erase

There is seen a problem sometimes where a flash loader run error occurs and is resolved after mass-erase
of the flash:

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

## Contributing and versioning

* The semantic versioning scheme is used. Read more at [semver.org](http://semver.org)
* When creating a pull request, please open first a issue for discussion of new features. Bugfixes don't need a discussion.

## License

The stlink library and tools are licensed under the [BSD license](LICENSE). With
some exceptions on external components (e.g flashloaders).
