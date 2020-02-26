Stlink ChangeLog
================

v1.6.0
======

Release date: 2020-02-20

Major changes and added features:

* Added O_BINARY option to open file ([#753](https://github.com/texane/stlink/pull/753))
* Added preliminary support for some STM32G0 chips ([#759](https://github.com/texane/stlink/pull/759), [#760](https://github.com/texane/stlink/pull/760))
* Added support for mass erasing second bank on STM32F10x_XL ([#767](https://github.com/texane/stlink/pull/767), [#768](https://github.com/texane/stlink/pull/768))
* Added call to clear PG bit after writing to flash ([#773](https://github.com/texane/stlink/pull/773))
* Added howto for sending NRST signal through GDB ([#774](https://github.com/texane/stlink/pull/774), [#776](https://github.com/texane/stlink/pull/776))
* Added support to write option bytes for the STM32G0 ([#778](https://github.com/texane/stlink/pull/778))
* Added simple read/write support for STM32WB55 chips ([#786](https://github.com/texane/stlink/pull/786))
* Added STLink V3SET VID:PIDs to the udev rules ([#789](https://github.com/texane/stlink/pull/789))
* Support for "STM32+Audio" v2-1 firmware ([#790](https://github.com/texane/stlink/pull/790))
* Initial support for STM32L41X ([#799](https://github.com/texane/stlink/pull/799))
* Build for Windows under Debian/Ubuntu ([#802](https://github.com/texane/stlink/pull/802))
* Allow for 64 bytes serials ([#809](https://github.com/texane/stlink/pull/809))
* Added support to read and write option bytes for STM32F2 series (Orie22)
* Added full support for STLINK CHIP ID L4RX (Brad Natelborg)
* Added support to write option bytes to STM32F4 devices (Davey Struijk)

Updates and fixes:

* Build failure when platform is 32 bit, but stuct stat.st_size is 64 bit. ([#629](https://github.com/texane/stlink/pull/629))
* Made udev rules and modprobe conf installation optional ([#741](https://github.com/texane/stlink/pull/741))
* Fixed case when __FILE__ don't contain "/" nor "\\". ([#745](https://github.com/texane/stlink/pull/745))
* Fixed double dash issue in doc/man ([#746](https://github.com/texane/stlink/pull/746))
* Fixed Debug error on line 123 in CMakeLists.txt (@xor-gate)
* Only do bank calculation on STM32L4 devices with dual banked flash ([#751](https://github.com/texane/stlink/pull/751))
* Updated STM32F3xx chip ID that covers a few different devices ([#758](https://github.com/texane/stlink/pull/758))
* Fixed versioning when compiling from the checked out git-repo ([#762](https://github.com/texane/stlink/pull/762))
* Fixed "unkown chip id", piped output and st-util -v ([#107](https://github.com/texane/stlink/pull/107), [#763](https://github.com/texane/stlink/pull/763))
* win32: move usleep definition to unistd.h ([#765](https://github.com/texane/stlink/pull/765))
* Fixed relative path to the UI files needed by stlink-gui-local (GUI) ([#770](https://github.com/texane/stlink/pull/770), [#771](https://github.com/texane/stlink/pull/771))
* Fixed package name "devscripts" in doc/compiling.md ([#775](https://github.com/texane/stlink/pull/775))
* Fixed apparent STM32G0 flashing issue ([#797](https://github.com/texane/stlink/pull/797))
* Fixed few potential memory/resource leaks ([#803](https://github.com/texane/stlink/pull/803))
* Fixed flash verification error on STM32WB55RG ([#810](https://github.com/texane/stlink/pull/810), [#816](https://github.com/texane/stlink/pull/816))
* Do not issue JTAG reset on stlink-v1 (Gwenhael Goavec-Merou)
* Fixed flash size of STM32 Discovery vl (Gwenhael Goavec-Merou)
* Added support for writing option bytes on STM32L0 (Adrian Imboden)
* Updated documentation on software structure ([#851](https://github.com/texane/stlink/pull/851))

General project updates:

* Updated issue templates, README.md and CHANGELOG.md (Nightwalker-87)
* Added CODE_OF_CONDUCT (Nightwalker-87)
* Fixed travis build config file (Nightwalker-87)
* Archived page from github project wiki to doc/wiki_old.md (Nightwalker-87)

v1.5.1
======

Release date: 2018-09-13

Major changes and added features:

* Added button to export stm32 flash memory to a file ([#691](https://github.com/texane/stlink/pull/691))
* Updated libusb to 1.0.22 ([#695](https://github.com/texane/stlink/pull/695))
* Added desktop file for linux ([#688](https://github.com/texane/stlink/pull/688))
* Added icons for stlink GUI ([#697](https://github.com/texane/stlink/pull/697))
* Added support for STM32L4R9 target ([#694](https://github.com/texane/stlink/pull/694), [#699](https://github.com/texane/stlink/pull/699))
* Added creation of icons for .desktop file ([#708](https://github.com/texane/stlink/pull/708))
* Added memory map for STM32F411RE target ([#709](https://github.com/texane/stlink/pull/709))
* Added reset through AIRCR ([#540](https://github.com/texane/stlink/pull/540), [#712](https://github.com/texane/stlink/pull/712))
* Implemented intel hex support for GTK GUI ([#718](https://github.com/texane/stlink/pull/718))

Fixes:

* Fixed missing flash_loader for L011 ([#675](https://github.com/texane/stlink/pull/675))
* Fixed serial number size mismatch with stlink_open_usb() ([#680](https://github.com/texane/stlink/pull/680))
* Debian packaging, CMake and README.md fixes ([#683](https://github.com/texane/stlink/pull/683))
* Fix for stlink library calls exit() or _exit() ([#634](https://github.com/texane/stlink/pull/634), [#696](https://github.com/texane/stlink/pull/696))
* Fix for libusb deprecation ([#703](https://github.com/texane/stlink/pull/703), [#704](https://github.com/texane/stlink/pull/704))
* Renamed STLINK_CHIPID_STM32_L4R9 to STLINK_CHIPID_STM32_L4RX ([#706](https://github.com/texane/stlink/pull/706))
* Fixed flash memory map for F72XXX target ([#711](https://github.com/texane/stlink/pull/711))
* Proper flash page size calculation for F412 target ([#721](https://github.com/texane/stlink/pull/721))
* Return correct value on EOF for Semihosting SYS_READ ([#726](https://github.com/texane/stlink/pull/726), [#729](https://github.com/texane/stlink/pull/729), [#731](https://github.com/texane/stlink/pull/731))
* Fix for mem_write() ([#730](https://github.com/texane/stlink/pull/730))
* FreeBSD defines LIBUSB_API_VERSION instead of LIBUSBX_API_VERSION ([#733](https://github.com/texane/stlink/pull/733))

v1.5.0
======

Release date: 2018-02-16

Major changes and added features:

* STM32F72xx73xx support ([#1969148](https://github.com/texane/stlink/commit/19691485359afef1a256964afcbb8dcf4b733209))
* Added support of STM32L496xx/4A6xx devices ([#615](https://github.com/texane/stlink/pull/615))

Fixes:

* Fixed memory map for stm32l496xx boards ([#639](https://github.com/texane/stlink/pull/639))
* Fixed write for microcontroller with RAM size less or equal to 32K ([#637](https://github.com/texane/stlink/pull/637))
* Added LIB_INSTALL_DIR to correct libs install on 64-bit systems ([#636](https://github.com/texane/stlink/pull/636))
* Fixed verification of flash error for STM32L496x device ([#618](https://github.com/texane/stlink/pull/618))
* Fixed build on Fedora with GCC 8 ([#666](https://github.com/texane/stlink/pull/668))

v1.4.0
======

Release date: 2017-07-01

Major changes and added features:

* Added support for STM32L452 target ([#608](https://github.com/texane/stlink/pull/608))
* Initial support to compile with Microsoft Visual Studio 2017 ([#602](https://github.com/texane/stlink/pull/602))
* Added support for flashing second bank on STM32F10x_XL ([#592](https://github.com/texane/stlink/pull/592))
* Added support for STM32L011 target ([#572](https://github.com/texane/stlink/pull/572))
* Allow building of debian package with CPack (@xor-gate)

Updates and fixes:

* Fixed compilation with GCC 7 ([#590](https://github.com/texane/stlink/pull/590))
* Skipped GTK detection if we're cross-compiling ([#588](https://github.com/texane/stlink/pull/588))
* Fixed possible memory leak ([#570](https://github.com/texane/stlink/pull/570))
* Fixed building with mingw64 ([#569](https://github.com/texane/stlink/pull/569), [#610](https://github.com/texane/stlink/pull/610))
* Updated libusb to 1.0.21 for Windows ([#562](https://github.com/texane/stlink/pull/562))
* Fixed low-voltage flashing on STM32F7 parts. ([#567](https://github.com/texane/stlink/pull/567))
* Updated libusb to 1.0.21 for Windows ([#562](https://github.com/texane/stlink/pull/562))

v1.3.1
======

Release date: 2017-02-25

Major changes and added features:

* Added preliminary support for STM32L011 to see it after probe (chipid `0x457`) (@xor-gate)
* Stripped full paths to source files in log (commit [#2c0ab7f](https://github.com/texane/stlink/commit/2c0ab7f0eb6cfda5cfbdc08bb9f6760d27c2b667))
* Added support for STM32F413 target ([#549](https://github.com/texane/stlink/pull/549))
* Added support for Semihosting `SYS_READC` ([#546](https://github.com/texane/stlink/pull/546))

Updates and fixes:

* Updated documentation markdown files
* Compilation fixes ([#552](https://github.com/texane/stlink/pull/552))
* Fixed compilation when path includes spaces ([#561](https://github.com/texane/stlink/pull/561))

v1.3.0
======

Release date: 2017-01-28

Major changes and added features:

* Deprecation of autotools (autoconf, automake) ([#83](https://github.com/texane/stlink/pull/83), [#431](https://github.com/texane/stlink/pull/431), [#434](https://github.com/texane/stlink/pull/434))
* Removal of undocumented `st-term` utility, which is now replaced by `st-util` ARM semihosting feature ([#3fd0f09](https://github.com/texane/stlink/commit/3fd0f099782506532198473b24f643a3f68d5ff9))
* Added support for native debian packaging ([#444](https://github.com/texane/stlink/pull/444), [#485](https://github.com/texane/stlink/pull/485))
* Added intel hex file reading for `st-flash` ([#110](https://github.com/texane/stlink/pull/110), [#157](https://github.com/texane/stlink/pull/157), [#200](https://github.com/texane/stlink/pull/200), [#239](https://github.com/texane/stlink/pull/239), [#457](https://github.com/texane/stlink/pull/547), [#459](https://github.com/texane/stlink/pull/549))
* Added `--reset` command to `st-flash` ([#505](https://github.com/texane/stlink/pull/505))
* Support serial numbers argument for `st-util` and `st-flash` for multi-programmer setups ([#541](https://github.com/texane/stlink/pull/541))
* Added kill ('k') command to gdb-server for `st-util` ([#9804416](https://github.com/texane/stlink/commit/98044163ab34bf5159f121d1c49ffb3550321ca0))
* Added manpages (generated with pandoc from Markdown) ([#464](https://github.com/texane/stlink/pull/464))
* Rewritten commandline parsing for `st-flash` ([#459](https://github.com/texane/stlink/pull/459))
* Added support for ARM semihosting to `st-util` ([#454](https://github.com/texane/stlink/pull/454), [#455](https://github.com/texane/stlink/pull/455))

Chip support added for:

* STM32L432 ([#501](https://github.com/texane/stlink/pull/501))
* STM32F412 ([#538](https://github.com/texane/stlink/pull/538))
* STM32F410 ([#9c635e4](https://github.com/texane/stlink/commit/9c635e419deca697ff823000aad2e39d47ec8d6c))
* Added memory map for STM32F401XE ([#460](https://github.com/texane/stlink/pull/460))
* L0x Category 5 devices ([#406](https://github.com/texane/stlink/pull/406))
* Added L0 Category 2 device (chip id: 0x425) ([#72b8e5e](https://github.com/texane/stlink/commit/72b8e5ec87e4fa386a8e94fe68df29467d4354ce))

Updates and fixes:

* Set SWDCLK and fixed jtag_reset bug ([#254](https://github.com/texane/stlink/pull/254), [#291](https://github.com/texane/stlink/pull/291), [#475](https://github.com/texane/stlink/pull/475), [#533](https://github.com/texane/stlink/pull/533), [#534](https://github.com/texane/stlink/pull/534))
* Fixed STM32F030 erase error ([#442](https://github.com/texane/stlink/pull/442))
* Fixed Cygwin build ([#68b0f3b](https://github.com/texane/stlink/commit/68b0f3bddc3c4aaffe34caa6a3201029edd8ad56))
* Reset flash mass erase (MER) bit after mass erase for safety ([#489](https://github.com/texane/stlink/pull/489))
* Fixed memory map for STM32F4 (@zulusw)
* Fixed STM32L-problem with flash loader ([#390](https://github.com/texane/stlink/pull/390))
* `st-util` don't read target voltage on startup as it crashes STM32F100 (probably stlink/v1) (Greg Alexander)
* Do a JTAG reset prior to reading CPU information when processor is in deep sleep (@andyg24)
* Redesign of `st-flash` commandline options parsing ([#459](https://github.com/texane/stlink/pull/459))

v1.2.0
======

Release date: 2016-05-16

Features added:

* Added multiple stlink probing (`st-info --probe`, `st-info --hla-serial`) with printing serial in hex and OpenOCD `hla_serial` format (Jerry Jacobs)
* Added stlink usb probe API functions (Jerry Jacobs)
* Added parameter to specify one stlink v2 of many (Georg von Zengen)

Changes:

* Refactoring/fixes of flash loader (Maxime Coquelin)

Updates and fixes:

* Synchronized cache for stm32f7 (Tristan Gingold)
* Allow flashing of STM32L4 down to 1.71 V (Greg Meiste)
* Fix on stm32l4 to clear flash mass erase flags on CR (Bruno Dal Bo)
* Proper writing of page 0 of second bank for stm32l476xe (Tobias Badertscher)
* Trace the read data in stlink_read_debug32 and not the address of the variable (Tobias Badertscher)
* Mac OS X El Capitan platform support confirmation (Nikolay)
* Do not send a NUL at end of packets to gdb (Tristan Gingold)
* Correctly compute flash write size for partial pages (Dave Vandervies)
* _stlink_usb_reset use hardreset (mlundinse)
* Make sure MCU is halted before running RAM based flashloaders (mlundinse)
* Could not flash STM32_F3_SMALL (Max Chen)
* STM32F4 8-bit support for 1.8v operation (Andy Isaacson)
* Fixed F2 memory map (Nicolas Schodet)
* Memory map for stm32f42xxx and stm32f43xxx devices (Craig Lilley)
* Stm32l0x flash loader (Robin Kreis)

Chip support added for:

* STM32L053R8 (Jean-Luc Béchennec)
* STM32F7 Support (mlundinse)
* Added STM32L4 to CHIPID #defines and devices[], flash driver and loader (Dave Vandervies)
* Basic support for F446 (Pavel Kirienko)
* STM32F303 High Density
* STM32L1xx Cat.2 devices (Nicolas Schodet)

Board support added for:

* Nucleo-F303RE (Kyle Manna)
* Nucleo-F411RE (texane)

Build system:

* Travis: Initial support for Travis continues integration on Linux & Mac OS X (Jerry Jacobs)
* CMake: Document in README.md and add extra strict compiler flags (Jerry Jacobs)
* CMake: First stab at a cmake build (Josh Bialkowski)
