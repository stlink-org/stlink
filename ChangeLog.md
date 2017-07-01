Stlink ChangeLog
================

v1.4.0
======

Release date: 2017-07-01

Major changes and added features:

* Add support for STM32L452 target ([#608](https://github.com/texane/stlink/pull/608))
* Initial support to compile with Microsoft Visual Studio 2017 ([#602](https://github.com/texane/stlink/pull/602))
* Added support for flashing second bank on STM32F10x_XL ([#592](https://github.com/texane/stlink/pull/592))
* Add support for STM32L011 target ([#572](https://github.com/texane/stlink/pull/572)) 
* Allow building of debian package with CPack (@xor-gate)

Updates and fixes:

* Fix compilation with GCC 7 ([#590](https://github.com/texane/stlink/pull/590))
* Skip GTK detection if we're cross-compiling ([#588](https://github.com/texane/stlink/pull/588))
* Fix possible memory leak ([#570](https://github.com/texane/stlink/pull/570))
* Fix building with mingw64 ([#569](https://github.com/texane/stlink/pull/569), [#610](https://github.com/texane/stlink/pull/610))
* Update libusb to 1.0.21 for Windows ([#562](https://github.com/texane/stlink/pull/562))
* Fixing low-voltage flashing on STM32F7 parts. ([#567](https://github.com/texane/stlink/pull/567))
* Update libusb to 1.0.21 for Windows ([#562](https://github.com/texane/stlink/pull/562))

v1.3.1
======

Release date: 2017-02-25

Major changes and added features:

* Add preliminary support for STM32L011 to see it after probe (chipid `0x457`) (@xor-gate)
* Strip full paths to source files in log (commit [#2c0ab7f](https://github.com/texane/stlink/commit/2c0ab7f0eb6cfda5cfbdc08bb9f6760d27c2b667))
* Add support for STM32F413 target ([#549](https://github.com/texane/stlink/pull/549))
* Add support for Semihosting `SYS_READC` ([#546](https://github.com/texane/stlink/pull/546))

Updates and fixes:

* Update documentation markdown files
* Compilation fixes ([#552](https://github.com/texane/stlink/pull/552))
* Fix compilation when path includes spaces ([#561](https://github.com/texane/stlink/pull/561))

v1.3.0
======

Release date: 2017-01-28

Major changes and added features:

* Deprecation of autotools (autoconf, automake) (@xor-gate)
* Removal of undocumented `st-term` utility, which is now replaced by `st-util` ARM semihosting feature ([#3fd0f09](https://github.com/texane/stlink/commit/3fd0f099782506532198473b24f643a3f68d5ff9))
* Add support for native debian packaging ([#444](https://github.com/texane/stlink/pull/444), [#485](https://github.com/texane/stlink/pull/485))
* Add intel hex file reading for `st-flash` ([#459](https://github.com/texane/stlink/pull/541))
* Add `--reset` command to `st-flash` ([#505](https://github.com/texane/stlink/pull/505))
* Support serial numbers argument for `st-util` and `st-flash` for multi-programmer setups ([#541](https://github.com/texane/stlink/pull/541))
* Add kill ('k') command to gdb-server for `st-util` ([#9804416](https://github.com/texane/stlink/commit/98044163ab34bf5159f121d1c49ffb3550321ca0))
* Add manpages (generated with pandoc from Markdown) ([#464](https://github.com/texane/stlink/pull/464))
* Rewrite commandline parsing for `st-flash` ([#459](https://github.com/texane/stlink/pull/459))
* Add support for ARM semihosting to `st-util` ([#454](https://github.com/texane/stlink/pull/454), [#455](https://github.com/texane/stlink/pull/455))

Chip support added for:

* STM32L432 ([#501](https://github.com/texane/stlink/pull/501))
* STM32F412 ([#538](https://github.com/texane/stlink/pull/538))
* STM32F410 ([#9c635e4](https://github.com/texane/stlink/commit/9c635e419deca697ff823000aad2e39d47ec8d6c))
* Add memory map for STM32F401XE ([#460](https://github.com/texane/stlink/pull/460))
* L0x Category 5 devices ([#406](https://github.com/texane/stlink/pull/406))
* Add L0 Category 2 device (chip id: 0x425) ([#72b8e5e](https://github.com/texane/stlink/commit/72b8e5ec87e4fa386a8e94fe68df29467d4354ce))

Updates and fixes:

* Fixed STM32F030 erase error ([#442](https://github.com/texane/stlink/pull/442))
* Fixed Cygwin build ([#68b0f3b](https://github.com/texane/stlink/commit/68b0f3bddc3c4aaffe34caa6a3201029edd8ad56))
* Reset flash mass erase (MER) bit after mass erase for safety ([#489](https://github.com/texane/stlink/pull/489))
* Fix memory map for STM32F4 (@zulusw)
* Fix STM32L-problem with flash loader (issue #390) (Tom de Boer)
* `st-util` don't read target voltage on startup as it crashes STM32F100 (probably stlink/v1) (Greg Alexander)
* Do a JTAG reset prior to reading CPU information when processor is in deep sleep (@andyg24)
* Redesign of `st-flash` commandline options parsing (pull-request #459) (@dev26th)

v1.2.0
======

Release date: 16 may 2016

Features added:

* Add multiple stlink probing (`st-info --probe`, `st-info --hla-serial`) with printing serial in hex and OpenOCD `hla_serial` format (Jerry Jacobs)
* Add stlink usb probe API functions (Jerry Jacobs)
* Added parameter to specify one stlink v2 of many (Georg von Zengen)

Changes:

* Refactoring/fixes of flash loader (Maxime Coquelin)

Updates and fixes:

* Synchronize cache for stm32f7 (Tristan Gingold)
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
* Fix F2 memory map (Nicolas Schodet)
* Memory map for stm32f42xxx and stm32f43xxx devices (Craig Lilley)
* Stm32l0x flash loader (Robin Kreis)

Chip support added for:

* STM32L053R8 (Jean-Luc Béchennec)
* STM32F7 Support (mlundinse)
* Add STM32L4 to CHIPID #defines and devices[], flash driver and loaded (Dave Vandervies)
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
