Stlink ChangeLog
================

v2.0.0
======

Ongoing development

Major changes:

* Deprecation of autotools (autoconf, automake)

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
