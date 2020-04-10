Stlink ChangeLog
================

v1.6.0
======

Release date: 2020-02-20

Major changes and added features:

* Initial support for STM32L41X ([#754](https://github.com/texane/stlink/pull/754), [#799](https://github.com/texane/stlink/pull/799))
* Working support for CKS32F103C8T6 and related CKS devices with Core-ID 0x2ba01477 ([#756](https://github.com/texane/stlink/pull/756), [#757](https://github.com/texane/stlink/pull/757), [#805](https://github.com/texane/stlink/pull/805), [#834](https://github.com/texane/stlink/pull/834), Regression-Fixes: [#761](https://github.com/texane/stlink/pull/761), [#766](https://github.com/texane/stlink/pull/766))
* Added preliminary support for some STM32G0 chips ([#759](https://github.com/texane/stlink/pull/759), [#760](https://github.com/texane/stlink/pull/760), [#797](https://github.com/texane/stlink/pull/797))
* Added support for mass erasing second bank on STM32F10x_XL ([#767](https://github.com/texane/stlink/pull/767), [#768](https://github.com/texane/stlink/pull/768))
* Added call to clear PG bit after writing to flash ([#773](https://github.com/texane/stlink/pull/773))
* Added support to write option bytes for the STM32G0 ([#778](https://github.com/texane/stlink/pull/778))
* Added support for STM32WB55 chips ([#786](https://github.com/texane/stlink/pull/786), [#810](https://github.com/texane/stlink/pull/810), [#816](https://github.com/texane/stlink/pull/816))
* Added STLink V3SET VID:PIDs to the udev rules ([#789](https://github.com/texane/stlink/pull/789))
* Support for "STM32+Audio" v2-1 firmware ([#790](https://github.com/texane/stlink/pull/790))
* Build for Windows under Debian/Ubuntu ([#802](https://github.com/texane/stlink/pull/802))
* Allow for 64 bytes serials ([#809](https://github.com/texane/stlink/pull/809))
* Added full support for STLINK CHIP ID L4RX ([#814](https://github.com/texane/stlink/pull/814), [#839](https://github.com/texane/stlink/pull/839))
* Added support for the STLink-v2.1 when flashed with no mass storage (PID 0x3752) ([#819](https://github.com/texane/stlink/pull/819), [#861](https://github.com/texane/stlink/pull/861))
* Added support for writing option bytes on STM32L0xx ([#830](https://github.com/texane/stlink/pull/830))
* Added support to read and write option bytes for STM32F2 series ([#836](https://github.com/texane/stlink/pull/836), [#837](https://github.com/texane/stlink/pull/837))
* Added support to read and write option bytes for STM32F446 ([#843](https://github.com/texane/stlink/pull/843))

Updates and fixes:

* Fixed "unkown chip id", piped output and st-util -v ([#107](https://github.com/texane/stlink/pull/107), [#665](https://github.com/texane/stlink/pull/665), [#763](https://github.com/texane/stlink/pull/763))
* Fixed an issue with versioning stuck at 1.4.0 for versions cloned with git ([#563](https://github.com/texane/stlink/pull/563), [#762](https://github.com/texane/stlink/pull/762), [#772](https://github.com/texane/stlink/pull/772))
* Updated STM32F3xx chip ID that covers a few different devices ([#685](https://github.com/texane/stlink/pull/685), [#758](https://github.com/texane/stlink/pull/758))
* Made udev rules and modprobe conf installation optional ([#741](https://github.com/texane/stlink/pull/741))
* Fixed case when __FILE__ don't contain "/" nor "\\" ([#745](https://github.com/texane/stlink/pull/745))
* Fixed double dash issue in doc/man ([#746](https://github.com/texane/stlink/pull/746), [#747](https://github.com/texane/stlink/pull/747))
* Compiling documentation: package is called libusb-1.0-0-dev on Debian ([#748](https://github.com/texane/stlink/pull/748))
* Only do bank calculation on STM32L4 devices with dual banked flash / Added chip-ID 0x464 for STM32L41xxx/L42xxx devices ([#751](https://github.com/texane/stlink/pull/751))
* Added O_BINARY option to open file ([#753](https://github.com/texane/stlink/pull/753))
* Fixed versioning when compiling from the checked out git-repo ([#762](https://github.com/texane/stlink/pull/762), [#772](https://github.com/texane/stlink/pull/772))
* win32: move usleep definition to unistd.h ([#765](https://github.com/texane/stlink/pull/765))
* Fixed relative path to the UI files needed by stlink-gui-local (GUI) ([#770](https://github.com/texane/stlink/pull/770), [#771](https://github.com/texane/stlink/pull/771))
* Added howto for sending NRST signal through GDB ([#774](https://github.com/texane/stlink/pull/774), [#776](https://github.com/texane/stlink/pull/776), [#779](https://github.com/texane/stlink/pull/779))
* Fixed package name "devscripts" in doc/compiling.md ([#775](https://github.com/texane/stlink/pull/775))
* Fixed few potential memory/resource leaks ([#803](https://github.com/texane/stlink/pull/803))
* Updated Linux source repositories in README.md: Debian and Ubuntu ([#821](https://github.com/texane/stlink/pull/821), [#835](https://github.com/texane/stlink/pull/835), [#859](https://github.com/texane/stlink/pull/859))
* Do not issue JTAG reset on stlink-v1 ([#828](https://github.com/texane/stlink/pull/828))
* Fixed flash size of STM32 Discovery vl ([#829](https://github.com/texane/stlink/pull/829))
* Updated documentation on software structure ([#851](https://github.com/texane/stlink/pull/851))

General project updates:

* Updated README.md, CHANGELOG.md and issue templates (Nightwalker-87)
* Fixed travis build config file (Nightwalker-87)
* Added CODE_OF_CONDUCT (Nightwalker-87)
* Archived page from github project wiki to doc/wiki_old.md (Nightwalker-87)


v1.5.1
======

Release date: 2018-09-13

Major changes and added features:

* Added reset through AIRCR ([#540](https://github.com/texane/stlink/pull/540), [#712](https://github.com/texane/stlink/pull/712))
* Added creation of icons for .desktop file ([#684](https://github.com/texane/stlink/pull/684), [#708](https://github.com/texane/stlink/pull/708))
* Added desktop file for linux ([#688](https://github.com/texane/stlink/pull/688))
* Added button to export STM32 flash memory to a file ([#691](https://github.com/texane/stlink/pull/691))
* Updated libusb to 1.0.22 ([#695](https://github.com/texane/stlink/pull/695)) - (related Bugs: [#438](https://github.com/texane/stlink/pull/438), [#632](https://github.com/texane/stlink/pull/632))
* Added icons for STLink GUI ([#697](https://github.com/texane/stlink/pull/697))
* Added support for STM32L4R9 target ([#694](https://github.com/texane/stlink/pull/694), [#699](https://github.com/texane/stlink/pull/699))
* Added memory map for STM32F411RE target ([#709](https://github.com/texane/stlink/pull/709))
* Implemented intel hex support for GTK GUI ([#713](https://github.com/texane/stlink/pull/713), [#718](https://github.com/texane/stlink/pull/718))

Updates and fixes:

* Fixed missing flash_loader for STM32L0x ([#269](https://github.com/texane/stlink/pull/269), [#274](https://github.com/texane/stlink/pull/274), [#654](https://github.com/texane/stlink/pull/654), [#675](https://github.com/texane/stlink/pull/675))
* Fix for stlink library calls exit() or _exit() ([#634](https://github.com/texane/stlink/pull/634), [#696](https://github.com/texane/stlink/pull/696))
* Added semihosting parameter documentation in doc/man ([#674](https://github.com/texane/stlink/pull/674))
* Fixed reference to non-exisiting st-term tool in doc/man ([#676](https://github.com/texane/stlink/pull/676))
* Fixed serial number size mismatch with stlink_open_usb() ([#680](https://github.com/texane/stlink/pull/680))
* Debian packaging, CMake and README.md fixes ([#682](https://github.com/texane/stlink/pull/682), [#683](https://github.com/texane/stlink/pull/683))
* Disabled static library installation by default ([#702](https://github.com/texane/stlink/pull/702))
* Fix for libusb deprecation ([#703](https://github.com/texane/stlink/pull/703), [#704](https://github.com/texane/stlink/pull/704))
* Renamed STLINK_CHIPID_STM32_L4R9 to STLINK_CHIPID_STM32_L4RX ([#706](https://github.com/texane/stlink/pull/706))
* Regression: stlink installation under Linux (Debian 9) is broken since #695 ([#700](https://github.com/texane/stlink/pull/700), [#701](https://github.com/texane/stlink/pull/701), [#707](https://github.com/texane/stlink/pull/707))
* Fixed flash memory map for STM32F72xxx target ([#711](https://github.com/texane/stlink/pull/711))
* Proper flash page size calculation for STM32F412xx target ([#721](https://github.com/texane/stlink/pull/721))
* Return correct value on EOF for Semihosting SYS_READ ([#726](https://github.com/texane/stlink/pull/726), [#727](https://github.com/texane/stlink/pull/727), [#728](https://github.com/texane/stlink/pull/728), [#729](https://github.com/texane/stlink/pull/729), [#730](https://github.com/texane/stlink/pull/730), [#731](https://github.com/texane/stlink/pull/731), [#732](https://github.com/texane/stlink/pull/732))
* FreeBSD defines LIBUSB_API_VERSION instead of LIBUSBX_API_VERSION ([#733](https://github.com/texane/stlink/pull/733))


v1.5.0
======

Release date: 2018-02-16

Major changes and added features:

* Added support of STM32L496xx/4A6xx devices ([#615](https://github.com/texane/stlink/pull/615), [#657](https://github.com/texane/stlink/pull/657))
* Added unknown chip dummy to obtain the serial of the ST-link by a call to st-info --probe ([#641](https://github.com/texane/stlink/pull/641))
* Added support for STM32F72xx (chip-ID: 0x452) devices (commit [#1969148](https://github.com/texane/stlink/commit/19691485359afef1a256964afcbb8dcf4b733209))

Updates and fixes:

* Fixed verification of flash error for STM32L496x device ([#617](https://github.com/texane/stlink/pull/617), [#618](https://github.com/texane/stlink/pull/618))
* Updated Linux source repositories in README.md: Gentoo, Fedora and RedHat/CentOS ([#622](https://github.com/texane/stlink/pull/622), [#635](https://github.com/texane/stlink/pull/635))
* Updated changelog in debian package ([#630](https://github.com/texane/stlink/pull/630))
* Added LIB_INSTALL_DIR to correct libs install on 64-bit systems ([#633](https://github.com/texane/stlink/pull/633), [#636](https://github.com/texane/stlink/pull/636))
* Fixed write for microcontroller with RAM size less or equal to 32K ([#637](https://github.com/texane/stlink/pull/637))
* Fixed memory map for STM32L496xx boards ([#639](https://github.com/texane/stlink/pull/639))
* Fixed __FILE__ base name extraction ([#624](https://github.com/texane/stlink/pull/624), [#628](https://github.com/texane/stlink/pull/628), [#648](https://github.com/texane/stlink/pull/648))
* Added debian/triggers to run ldconfig ([#664](https://github.com/texane/stlink/pull/664))
* Fixed build on Fedora with GCC 8 ([#666](https://github.com/texane/stlink/pull/666), [#667](https://github.com/texane/stlink/pull/667), [#668](https://github.com/texane/stlink/pull/668))


v1.4.0
======

Release date: 2017-07-01

Major changes and added features:

* Allow building of debian package with CPack ([#554](https://github.com/texane/stlink/pull/554), commit [#5b69f25](https://github.com/texane/stlink/commit/5b69f25198a1a8f34e2ee48d1ad20f79447e3d55))
* Added support for STM32L011 target ([#564](https://github.com/texane/stlink/pull/564), [#565](https://github.com/texane/stlink/pull/565), [#572](https://github.com/texane/stlink/pull/572))
* Added support for flashing second bank on STM32F10x_XL ([#592](https://github.com/texane/stlink/pull/592))
* Initial support to compile with Microsoft Visual Studio 2017 ([#602](https://github.com/texane/stlink/pull/602))
* Added support for STM32L452 target ([#603](https://github.com/texane/stlink/pull/603), [#608](https://github.com/texane/stlink/pull/608))

Updates and fixes:

* Fixed gdb-server: STM32L0xx has no FP_CTRL register for breakpoints ([#273](https://github.com/texane/stlink/pull/273))
* Added --flash=n[k][m] command line option to override device model ([#305](https://github.com/texane/stlink/pull/305), [#516](https://github.com/texane/stlink/pull/516), [#576](https://github.com/texane/stlink/pull/576))
* Updated libusb to 1.0.21 for Windows ([#562](https://github.com/texane/stlink/pull/562))
* Fixed low-voltage flashing on STM32F7 devices ([#566](https://github.com/texane/stlink/pull/566), [#567](https://github.com/texane/stlink/pull/567))
* Fixed building with mingw64 ([#569](https://github.com/texane/stlink/pull/569), [#573](https://github.com/texane/stlink/pull/573), [#578](https://github.com/texane/stlink/pull/578), [#582](https://github.com/texane/stlink/pull/582), [#584](https://github.com/texane/stlink/pull/584), [#610](https://github.com/texane/stlink/pull/610), [#846](https://github.com/texane/stlink/pull/846))
* Fixed possible memory leak ([#570](https://github.com/texane/stlink/pull/570), [#571](https://github.com/texane/stlink/pull/571))
* Fixed installation path for shared objects ([#581](https://github.com/texane/stlink/pull/581))
* Fixed a few -Wformat warnings ([#582](https://github.com/texane/stlink/pull/582))
* Removed unused defines in mimgw.h ([#583](https://github.com/texane/stlink/pull/583))
* Skip GTK detection when cross-compiling ([#588](https://github.com/texane/stlink/pull/588))
* Fixed compilation with GCC 7 ([#590](https://github.com/texane/stlink/pull/590), [#591](https://github.com/texane/stlink/pull/591))
* Fixed flashing to 'f0 device' targets ([#594](https://github.com/texane/stlink/pull/594), [#595](https://github.com/texane/stlink/pull/595))
* Fixed wrong counting when flashing ([#605](https://github.com/texane/stlink/pull/605))


v1.3.1
======

Release date: 2017-02-25

Major changes and added features:

* Added support for Semihosting `SYS_READC` ([#546](https://github.com/texane/stlink/pull/546))
* Added support for STM32F413 ([#549](https://github.com/texane/stlink/pull/549), [#550](https://github.com/texane/stlink/pull/550), [#758](https://github.com/texane/stlink/pull/758))
* Added preliminary support for STM32L011 to see it after probe (chip-ID 0x457) ([#558](https://github.com/texane/stlink/pull/558), [#598](https://github.com/texane/stlink/pull/598))

Updates and fixes:

* cmake/CPackConfig.cmake: Fixup OSX zip filename
* Updated source repositories in README.md: Windows, macOS, Alpine Linux
* Compilation fixes ([#547](https://github.com/texane/stlink/pull/547), [#551](https://github.com/texane/stlink/pull/551), [#552](https://github.com/texane/stlink/pull/552))
* Stripped full paths to source files in log ([#548](https://github.com/texane/stlink/pull/548))
* Fixed incorrect release folder name in docs ([#560](https://github.com/texane/stlink/pull/560))
* Fixed compilation when path includes spaces ([#561](https://github.com/texane/stlink/pull/561))


v1.3.0
======

Release date: 2017-01-28

Major changes and added features:

* Deprecation of autotools (autoconf, automake) and fixed build with MinGW ([#83](https://github.com/texane/stlink/pull/83), [#431](https://github.com/texane/stlink/pull/431), [#434](https://github.com/texane/stlink/pull/434), [#465](https://github.com/texane/stlink/pull/465))
* Added intel hex file reading for `st-flash` ([#110](https://github.com/texane/stlink/pull/110), [#157](https://github.com/texane/stlink/pull/157), [#200](https://github.com/texane/stlink/pull/200), [#239](https://github.com/texane/stlink/pull/239), [#457](https://github.com/texane/stlink/pull/547), [#459](https://github.com/texane/stlink/pull/549))
* Added support for ARM semihosting to `st-util` ([#147](https://github.com/texane/stlink/pull/147), [#227](https://github.com/texane/stlink/pull/227), [#454](https://github.com/texane/stlink/pull/454), [#455](https://github.com/texane/stlink/pull/455))
* Added manpages (generated with pandoc from Markdown) ([#208](https://github.com/texane/stlink/pull/208), [#464](https://github.com/texane/stlink/pull/464), [#466](https://github.com/texane/stlink/pull/466), [#467](https://github.com/texane/stlink/pull/467))
* Removal of undocumented `st-term` utility, which is now replaced by `st-util` ARM semihosting feature ([#228](https://github.com/texane/stlink/pull/228), ([#507](https://github.com/texane/stlink/pull/507), commit [#3fd0f09](https://github.com/texane/stlink/commit/3fd0f099782506532198473b24f643a3f68d5ff9))
* Support serial numbers argument for `st-util` and `st-flash` to probe and control multiple connected programmers ([#318](https://github.com/texane/stlink/pull/318), [#398](https://github.com/texane/stlink/pull/398), [#541](https://github.com/texane/stlink/pull/541))
* Merge st-probe tool into st-info ([#398](https://github.com/texane/stlink/pull/398))
* Added support for native debian packaging ([#444](https://github.com/texane/stlink/pull/444), [#472](https://github.com/texane/stlink/pull/472), [#473](https://github.com/texane/stlink/pull/473), [#482](https://github.com/texane/stlink/pull/482), [#483](https://github.com/texane/stlink/pull/483), [#484](https://github.com/texane/stlink/pull/484), [#485](https://github.com/texane/stlink/pull/485))
* Rewritten commandline parsing for `st-flash` ([#459](https://github.com/texane/stlink/pull/459))
* Added `--reset` command to `st-flash` ([#505](https://github.com/texane/stlink/pull/505))
* st-util should detect when USB commands fail ([#525](https://github.com/texane/stlink/pull/525), ([#527](https://github.com/texane/stlink/pull/527), ([#528](https://github.com/texane/stlink/pull/528))

Chip support added for:

* STM32F401XE: Added memory map for device ([#460](https://github.com/texane/stlink/pull/460))
* STM32F410RBTx ([#418](https://github.com/texane/stlink/pull/418))
* STM32F412 ([#537](https://github.com/texane/stlink/pull/537), [#538](https://github.com/texane/stlink/pull/538))
* STM32F7xx ([#324](https://github.com/texane/stlink/pull/324), [#326](https://github.com/texane/stlink/pull/326), [#327](https://github.com/texane/stlink/pull/327), [#337](https://github.com/texane/stlink/pull/337))
* STM32F767ZI ([#509](https://github.com/texane/stlink/pull/509))
* STM32L0xx Cat2 devices (chip-ID: 0x425) ([#414](https://github.com/texane/stlink/pull/414))
* STM32L0xx Cat5 devices (chip-ID: 0x447) ([#387](https://github.com/texane/stlink/pull/387), [#406](https://github.com/texane/stlink/pull/406))
* STM32L4xx ([#321](https://github.com/texane/stlink/pull/321))
* STM32L432 ([#500](https://github.com/texane/stlink/pull/500), [#501](https://github.com/texane/stlink/pull/501))

Updates and fixes:

* Fixed "unaligned addr or size" when trying to write a program in RAM ([#323](https://github.com/texane/stlink/pull/323))
* Fixed flashing on STM32_F3_SMALL ([#325](https://github.com/texane/stlink/pull/325))
* Fixed STM32L-problem with flash loader ([#390](https://github.com/texane/stlink/pull/390), [#407](https://github.com/texane/stlink/pull/407),[#408](https://github.com/texane/stlink/pull/408))
* Don't read the target voltage on startup, because it crashes STM32F100 ([#423](https://github.com/texane/stlink/pull/423), [#424](https://github.com/texane/stlink/pull/424))
* Added a useful error message instead of "[!] send_recv" ([#425](https://github.com/texane/stlink/pull/425), [#426](https://github.com/texane/stlink/pull/426))
* Do a JTAG reset prior to reading CPU information when processor is in deep sleep ([#428](https://github.com/texane/stlink/pull/428), [#430](https://github.com/texane/stlink/pull/430), [#451](https://github.com/texane/stlink/pull/451))
* Fixed STM32F030 erase error ([#442](https://github.com/texane/stlink/pull/442))
* Fixed memory map for STM32F7xx ([#453](https://github.com/texane/stlink/pull/453), [#456](https://github.com/texane/stlink/pull/456))
* Redesign of `st-flash` commandline options parsing ([#459](https://github.com/texane/stlink/pull/459))
* Set SWDCLK and fixed jtag_reset bug ([#475](https://github.com/texane/stlink/pull/475), [#534](https://github.com/texane/stlink/pull/534))
* doc/compiling.md: Add note about installation and ldconfig ([#478](https://github.com/texane/stlink/pull/478), commit [#be66bbf](https://github.com/texane/stlink/commit/be66bbf200c718904514b044ba84d64a36456218))
* Fixed Release target to generate the man-pages with pandoc ([#479](https://github.com/texane/stlink/pull/479))
* Fixed Cygwin build ([#487](https://github.com/texane/stlink/pull/487), ([#506](https://github.com/texane/stlink/pull/506))
* Reset flash mass erase (MER) bit after mass erase for safety ([#489](https://github.com/texane/stlink/pull/489))
* Wrong extract command in FindLibUSB.cmake ([#510](https://github.com/texane/stlink/pull/510), [#511](https://github.com/texane/stlink/pull/511))
* Fixed compilation error on Ubuntu 16.10 ([#514](https://github.com/texane/stlink/pull/514), [#525](https://github.com/texane/stlink/pull/525))


v1.2.0
======

Release date: 2016-05-16

Features added:

* Added multiple stlink probing (`st-info --probe`, `st-info --hla-serial`) with printing serial in hex and OpenOCD `hla_serial` format (Jerry Jacobs)
* Added stlink usb probe API functions (Jerry Jacobs)
* Added parameter to specify one stlink v2 of many (Georg von Zengen)

Updates and fixes:

* Refactoring/fixes of flash loader (Maxime Coquelin)
* Synchronized cache for STM32F7 (Tristan Gingold)
* Allow flashing of STM32L4 down to 1.71 V (Greg Meiste)
* Fix on STM32L4 to clear flash mass erase flags on CR (Bruno Dal Bo)
* Proper writing of page 0 of second bank for stm32l476xe (Tobias Badertscher)
* Trace the read data in stlink_read_debug32 and not the address of the variable (Tobias Badertscher)
* Mac OS X El Capitan platform support confirmation (Nikolay)
* Do not send a NUL at end of packets to gdb (Tristan Gingold)
* Correctly compute flash write size for partial pages (Dave Vandervies)
* _stlink_usb_reset use hardreset (mlundinse)
* Make sure MCU is halted before running RAM based flashloaders (mlundinse)
* Could not flash STM32_F3_SMALL (Max Chen)
* STM32F4 8-bit support for 1.8v operation (Andy Isaacson)
* Fixed STM32F2xx memory map (Nicolas Schodet)
* Memory map for STM32F42xxx and STM32F43xxx devices (Craig Lilley)
* Stm32l0x flash loader (Robin Kreis)
* Send F4 memory-map and features for STM32F429 ([#188](https://github.com/texane/stlink/pull/188), [#196](https://github.com/texane/stlink/pull/196), [#250](https://github.com/texane/stlink/pull/250), [#251](https://github.com/texane/stlink/pull/251)) (Release v1.1.0)
* Added AHB3 Peripherals definition for STM32F4 ([#218](https://github.com/texane/stlink/pull/218), [#288](https://github.com/texane/stlink/pull/288)) (Release v1.1.0)
* Corrected flash size register address for STM32F2 devices ([#278](https://github.com/texane/stlink/pull/278)) (Release v1.0.0)

Chip support added for:

* STM32L053R8 (Jean-Luc Béchennec)
* STM32F7 Support (mlundinse)
* Added STM32L4 to CHIPID #defines and devices[], flash driver and loader (Dave Vandervies)
* Basic support for STM32F446 (Pavel Kirienko)
* STM32F303 High Density
* STM32F469/STM32F479 ([#345](https://github.com/texane/stlink/pull/345), [#555](https://github.com/texane/stlink/pull/555)) (Release v1.2.0)
* STM32L1xx Cat.2 devices (Nicolas Schodet)
* STM32L1xx (chip-ID 0x427) ([#152](https://github.com/texane/stlink/pull/152), [#163](https://github.com/texane/stlink/pull/163), [#165](https://github.com/texane/stlink/pull/165)) (Release v1.0.0)

Board support added for:

* Nucleo-F303RE (Kyle Manna)
* Nucleo-F411RE (texane)

Build system:

* Travis: Initial support for Travis continues integration on Linux & Mac OS X (Jerry Jacobs)
* CMake: Document in README.md and add extra strict compiler flags (Jerry Jacobs)
* CMake: First stab at a cmake build (Josh Bialkowski)
