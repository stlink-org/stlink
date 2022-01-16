# stlink Changelog

# v1.7.1

Release date: 2022-xx-xx

This release drops support for some older operating systems. Check project README for details.

Updated system requirements:
- `cmake` >= 3.10.2
- `libusb` >= 1.0.21
- `libgtk-dev` >= 3.22.30

Features:

- Support for writing option bytes on STM32F0/F1/F3 ([#346](https://github.com/stlink-org/stlink/pull/346), [#458](https://github.com/stlink-org/stlink/pull/458), [#808](https://github.com/stlink-org/stlink/pull/808), [#1084](https://github.com/stlink-org/stlink/pull/1084), [#1112](https://github.com/stlink-org/stlink/pull/1112))
- Added chip-IDs for STM32G0B0/G0B1/G0C1/G050/G051/G061 ([#1140](https://github.com/stlink-org/stlink/pull/1140))
- Added option byte info for STM32F411XX ([#1141](https://github.com/stlink-org/stlink/pull/1141))
- Expanded and revised list of chips ([#1145](https://github.com/stlink-org/stlink/pull/1145), [#1164](https://github.com/stlink-org/stlink/pull/1164))
- [STM32H72X/3X]: Added full access to all device memory ([#1158](https://github.com/stlink-org/stlink/pull/1158), [#1159](https://github.com/stlink-org/stlink/pull/1159))
- Added support for STM32WLEx ([#1173](https://github.com/stlink-org/stlink/pull/1173))
- Added support for STLINK-V3 devices with no MSD ([#1185](https://github.com/stlink-org/stlink/pull/1185))
- Updated gdb-server.c to allow external memory access on STM32H73xx ([#1196](https://github.com/stlink-org/stlink/pull/1196), [#1197](https://github.com/stlink-org/stlink/pull/1197))
- Erase addr size / section of the flash memory with st-flash ([#1213](https://github.com/stlink-org/stlink/pull/1213)) 

Updates & changes:

- [refactoring] Moved chip-specific parameters into separate files ([#237](https://github.com/stlink-org/stlink/pull/237), [#1129](https://github.com/stlink-org/stlink/pull/1129))
- Added instructions for bug-reports and feature-requests to contribution guidelines ([#906](https://github.com/stlink-org/stlink/pull/906))
- Added travis CI configuration for macOS 10.14 to maintain capability for 32-bit compilation ([#f5ada94](https://github.com/stlink-org/stlink/commit/f5ada9474cdb87ff37de0d4eb9e75622b5870646))
- Updated description of chip id 0x0457 to L01x/L02x ([#1143](https://github.com/stlink-org/stlink/pull/1143), [#1144](https://github.com/stlink-org/stlink/pull/1144))
- Dropped execute bits from source code files ([#1167](https://github.com/stlink-org/stlink/pull/1167))
- Use proper Markdown headers for supported MCUs ([#1168](https://github.com/stlink-org/stlink/pull/1168))
- Removed redundant array ([#1178](https://github.com/stlink-org/stlink/pull/1178))
- Updated chip config files from the library structs ([#1181](https://github.com/stlink-org/stlink/pull/1181))
- [doc] Corrected file path in tutorial ([#1186](https://github.com/stlink-org/stlink/pull/1186))
- Improved chipid checks and printouts ([#1188](https://github.com/stlink-org/stlink/pull/1188)) 

Fixes:
- cmake: Install shared libraries in proper directories ([#1142](https://github.com/stlink-org/stlink/pull/1142))
- Fixed clearance of the H7 dual bank flag ([#1146](https://github.com/stlink-org/stlink/pull/1146), [#1147](https://github.com/stlink-org/stlink/pull/1147))
- Fix for 'libusb_devices were leaked' when no ST-LINK programmer was found ([#1150](https://github.com/stlink-org/stlink/pull/1150))
- Set of fixes and improvements ([#1154](https://github.com/stlink-org/stlink/pull/1154))
- Removed limit check for WRITEMEM_32BIT ([#1157](https://github.com/stlink-org/stlink/pull/1157))
- Fixed get_stm32l0_flash_base address for STM32L152RE ([#1161](https://github.com/stlink-org/stlink/pull/1161), [#1162](https://github.com/stlink-org/stlink/pull/1162))
- Fixed segfault if chip was not found in chip config files ([#1138](https://github.com/stlink-org/stlink/pull/1138), [#1163](https://github.com/stlink-org/stlink/pull/1163), [#1165](https://github.com/stlink-org/stlink/pull/1165), [#1166](https://github.com/stlink-org/stlink/pull/1166), [#1170](https://github.com/stlink-org/stlink/pull/1170))
- Fixed parsing hex numbers in chip config files ([#1169](https://github.com/stlink-org/stlink/pull/1169))
- Corrected flash_pagesize to use hex format ([#1172](https://github.com/stlink-org/stlink/pull/1172))
- Fixed compilation for MSVC ([#1176](https://github.com/stlink-org/stlink/pull/1176))
- Fixed few warnings for msvc about type conversion with possible lost data ([#1179](https://github.com/stlink-org/stlink/pull/1179))
- st-flash and other utilities search for chip files in the wrong directory ([#1180](https://github.com/stlink-org/stlink/pull/1180), commit [#c8fc656](https://github.com/stlink-org/stlink/commit/c8fc6561fead79ad49c09d82bab864745086792c))
- Fixed broken build on 32 bit systems ([#985](https://github.com/stlink-org/stlink/pull/985), [#1175](https://github.com/stlink-org/stlink/pull/1175), commit [#c8fc656](https://github.com/stlink-org/stlink/commit/c8fc6561fead79ad49c09d82bab864745086792c))
- Define 'SSIZE_MAX' if not defined ([#1183](https://github.com/stlink-org/stlink/pull/1183))
- Fixed compliation for OpenBSD 7.0 ([#1202](https://github.com/stlink-org/stlink/pull/1202))
- Included 'SSIZE_MAX' from 'limits.h' in 'src/common.c' ([#1207](https://github.com/stlink-org/stlink/pull/1207))

# v1.7.0

Release date: 2021-04-25

This release drops support for the STLINK/V1 programmer on macOS 10.13.

Features:

- Extended set of cmd line arguments for st-info and st-util ([#332](https://github.com/stlink-org/stlink/pull/332), [#990](https://github.com/stlink-org/stlink/pull/990), [#1091](https://github.com/stlink-org/stlink/pull/1091), [#1114](https://github.com/stlink-org/stlink/pull/1114))
- Extended support for STM32H7 & rework of software reset ([#532](https://github.com/stlink-org/stlink/pull/532), [#801](https://github.com/stlink-org/stlink/pull/801), [#868](https://github.com/stlink-org/stlink/pull/868), [#1008](https://github.com/stlink-org/stlink/pull/1008), [#1059](https://github.com/stlink-org/stlink/pull/1059), [#1063](https://github.com/stlink-org/stlink/pull/1063), [#1071](https://github.com/stlink-org/stlink/pull/1071))
- Added support for STM32H742/743/753 ([#671](https://github.com/stlink-org/stlink/pull/671), [#793](https://github.com/stlink-org/stlink/pull/793), [#823](https://github.com/stlink-org/stlink/pull/823), [#998](https://github.com/stlink-org/stlink/pull/998), [#1052](https://github.com/stlink-org/stlink/pull/1052))
- Official support for STLINK-V3 programmers (commit [#5e0a502](https://github.com/stlink-org/stlink/commit/5e0a502df812495bfa96fa9116a19f1306152b17), [#820](https://github.com/stlink-org/stlink/pull/820), [#1022](https://github.com/stlink-org/stlink/pull/1022), [#1025](https://github.com/stlink-org/stlink/pull/1025))
- Added preliminary support for STM32L5x2 ([#904](https://github.com/stlink-org/stlink/pull/904), [#999](https://github.com/stlink-org/stlink/pull/999))
- Option bytes on the STM32F767 ZIT6 Nucleo-144 ([#968](https://github.com/stlink-org/stlink/pull/968), [#997](https://github.com/stlink-org/stlink/pull/997))
- Use SetConsoleCtrlHandler for Windows ([#1021](https://github.com/stlink-org/stlink/pull/1021))
- Increase STM32L0 `option_size` to 20 ([#1046](https://github.com/stlink-org/stlink/pull/1046))
- `st-util`: Add specialized memory map for STM32H7 devices ([#1060](https://github.com/stlink-org/stlink/pull/1060))
- Support for STM32F4 option bytes ([#1062](https://github.com/stlink-org/stlink/pull/1062))
- Link for WIN32 & APPLE with stlink-static ([#1069](https://github.com/stlink-org/stlink/pull/1069))
- ITM functionality for STLink/V2 and STM32Fxx chipsets ([#136](https://github.com/stlink-org/stlink/pull/136), [#179](https://github.com/stlink-org/stlink/pull/179), [#815](https://github.com/stlink-org/stlink/pull/815), [#1072](https://github.com/stlink-org/stlink/pull/1072))
- Included ITM functionality for building with MSVC ([#1080](https://github.com/stlink-org/stlink/pull/1080))
- Update for CI integration (commit [#0eebc9a](https://github.com/stlink-org/stlink/commit/0eebc9a74506e84d5c460ec325ae98064a81885e), [#1118](https://github.com/stlink-org/stlink/pull/1118))

Updates & changes:

- [doc] Added tutorial section on unknown chip id error (commit [#229c721](https://github.com/stlink-org/stlink/commit/229c721189587760db5509c59b3c02e93e7035c8), [#107](https://github.com/stlink-org/stlink/pull/107), [#568](https://github.com/stlink-org/stlink/pull/568))
- [doc] Updated documentation on target resetting ([#261](https://github.com/stlink-org/stlink/pull/261), [#533](https://github.com/stlink-org/stlink/pull/533), [#1107](https://github.com/stlink-org/stlink/pull/1107))
- [doc] Added note on `(gdb) run` command (commit [#03793d4](https://github.com/stlink-org/stlink/commit/03793d42b6078344a9ef8ad55f1d5d0fc19e486e), [#267](https://github.com/stlink-org/stlink/pull/267))
- [doc] `st-flash --reset` parameter (one solution for #356) ([#642](https://github.com/stlink-org/stlink/pull/642))
- [refactoring] General maintenance ([#864](https://github.com/stlink-org/stlink/pull/864), [#976](https://github.com/stlink-org/stlink/pull/976), [#978](https://github.com/stlink-org/stlink/pull/978))
- Imported debian pkg-settings ([#986](https://github.com/stlink-org/stlink/pull/986))
- Add support for FreeBSD's `libusb` reimplementation ([#992](https://github.com/stlink-org/stlink/pull/992), [#993](https://github.com/stlink-org/stlink/pull/993))
- [doc] Added explanation about STM32F103 fake chips (commit [#a66557a](https://github.com/stlink-org/stlink/commit/a66557a102d48e69feb0a9746e8e42c4baf31fe2), [#1024](https://github.com/stlink-org/stlink/pull/1024))
- [doc] Added example for output of `st-info --probe` ([#1007](https://github.com/stlink-org/stlink/pull/1007), [#1049](https://github.com/stlink-org/stlink/pull/1049))
- [refactoring] Correctly handle endianness without reference to host platform ([#1081](https://github.com/stlink-org/stlink/pull/1081))
- Check format string for log messages ([#1093](https://github.com/stlink-org/stlink/pull/1093))
- Removed abort() from stlink-lib ([#1116](https://github.com/stlink-org/stlink/pull/1116))

Fixes:

- Improvements and fixes of the flash loaders, unification of the reset function ([#244](https://github.com/stlink-org/stlink/pull/244), [#382](https://github.com/stlink-org/stlink/pull/382), [#705](https://github.com/stlink-org/stlink/pull/705), [#724](https://github.com/stlink-org/stlink/pull/724), [#980](https://github.com/stlink-org/stlink/pull/980), [#995](https://github.com/stlink-org/stlink/pull/995), [#1008](https://github.com/stlink-org/stlink/pull/1008), [#1115](https://github.com/stlink-org/stlink/pull/1115), [#1117](https://github.com/stlink-org/stlink/pull/1117), [#1122](https://github.com/stlink-org/stlink/pull/1122), [#1124](https://github.com/stlink-org/stlink/pull/1124))
- Flash loader rework ([#356](https://github.com/stlink-org/stlink/pull/356), [#556](https://github.com/stlink-org/stlink/pull/556), [#593](https://github.com/stlink-org/stlink/pull/593), [#597](https://github.com/stlink-org/stlink/pull/597), [#607](https://github.com/stlink-org/stlink/pull/607), [#612](https://github.com/stlink-org/stlink/pull/612), [#638](https://github.com/stlink-org/stlink/pull/638), [#661](https://github.com/stlink-org/stlink/pull/661), [#690](https://github.com/stlink-org/stlink/pull/690), [#724](https://github.com/stlink-org/stlink/pull/724), [#807](https://github.com/stlink-org/stlink/pull/807), [#817](https://github.com/stlink-org/stlink/pull/817), [#818](https://github.com/stlink-org/stlink/pull/818), [#854](https://github.com/stlink-org/stlink/pull/854), [#868](https://github.com/stlink-org/stlink/pull/868), [#967](https://github.com/stlink-org/stlink/pull/967), [#979](https://github.com/stlink-org/stlink/pull/979), [#1008](https://github.com/stlink-org/stlink/pull/1008), [#1043](https://github.com/stlink-org/stlink/pull/1043), [#1054](https://github.com/stlink-org/stlink/pull/1054), [#1092](https://github.com/stlink-org/stlink/pull/1092), [#1105](https://github.com/stlink-org/stlink/pull/1105), [#1113](https://github.com/stlink-org/stlink/pull/1113))
- Fixed old DFU serial number for STLINK programmers ([#417](https://github.com/stlink-org/stlink/pull/417), [#494](https://github.com/stlink-org/stlink/pull/494), [#1106](https://github.com/stlink-org/stlink/pull/1106), [#1121](https://github.com/stlink-org/stlink/pull/1121))
- Improvements for Chip_ID read ([#620](https://github.com/stlink-org/stlink/pull/620), [#1008](https://github.com/stlink-org/stlink/pull/1008), [#1120](https://github.com/stlink-org/stlink/pull/1120))
- Use vl flashloader for all STM32F1 series ([#724](https://github.com/stlink-org/stlink/pull/724), [#769](https://github.com/stlink-org/stlink/pull/769), [#1041](https://github.com/stlink-org/stlink/pull/1041), [#1044](https://github.com/stlink-org/stlink/pull/1044))
- [regression] Changed timeout on flash write ([#787](https://github.com/stlink-org/stlink/pull/787), [#981](https://github.com/stlink-org/stlink/pull/981), [#987](https://github.com/stlink-org/stlink/pull/987))
- cmake compile failure with external `CMAKE_MODULE_PATH` set ([#962](https://github.com/stlink-org/stlink/pull/962))
- doc/man: Fixed installation directory ([#970](https://github.com/stlink-org/stlink/pull/970))
- Fixed installation path for desktop-file and icons ([#972](https://github.com/stlink-org/stlink/pull/972))
- Fix for static linking of `libssp` ([#973](https://github.com/stlink-org/stlink/pull/973), [#974](https://github.com/stlink-org/stlink/pull/974))
- [regression] Fixed wrong formatting for library install path ([#978](https://github.com/stlink-org/stlink/pull/978), [#1089](https://github.com/stlink-org/stlink/pull/1089))
- Fixed installation of header files needed for compiling with `libstlink.so.1.6.1` (commit [#31b1fa1](https://github.com/stlink-org/stlink/commit/31b1fa16201521e2aaf464576f2f169981abede0), [#982](https://github.com/stlink-org/stlink/pull/982))
- Fixed `connect under reset` for `st-flash` and `st-util` ([#983](https://github.com/stlink-org/stlink/pull/983))
- Fix for `mmap() size_t overflow` in `st-flash` ([#988](https://github.com/stlink-org/stlink/pull/988), [#989](https://github.com/stlink-org/stlink/pull/989))
- [regression] `stlink-gui` installation issue on Ubuntu-18.04 ([#1001](https://github.com/stlink-org/stlink/pull/1001), [#1004](https://github.com/stlink-org/stlink/pull/1004), [#1006](https://github.com/stlink-org/stlink/pull/1006))
- `st-util`: wrong register values passed to `gdb` (STLink/V2) ([#1002](https://github.com/stlink-org/stlink/pull/1002), [#1011](https://github.com/stlink-org/stlink/pull/1011), [#1026](https://github.com/stlink-org/stlink/pull/1026), [#1027](https://github.com/stlink-org/stlink/pull/1027), [#1038](https://github.com/stlink-org/stlink/pull/1038), [#1064](https://github.com/stlink-org/stlink/pull/1064), [#1065](https://github.com/stlink-org/stlink/pull/1065))
- GDB: Fixed problems with target description ([#1013](https://github.com/stlink-org/stlink/pull/1013), [#1088](https://github.com/stlink-org/stlink/pull/1088), [#1109](https://github.com/stlink-org/stlink/pull/1109))
- [doc] Fixed wrong path for `rules.d` folder ([#1020](https://github.com/stlink-org/stlink/pull/1020))
- Fixed support for STLINK/V1 programmer ([#1045](https://github.com/stlink-org/stlink/pull/1045), [#1105](https://github.com/stlink-org/stlink/pull/1105))
- st-util v1.6.1 does not recognize option --freq (commit [#e576768](https://github.com/stlink-org/stlink/commit/e5767681f14de9851aa970a9299930ca68b2ed92), [#1055](https://github.com/stlink-org/stlink/pull/1055))
- Fixed `gettimeofday` for MSVC ([#1074](https://github.com/stlink-org/stlink/pull/1074))
- Bugfixes for compilation with clang ([#1076](https://github.com/stlink-org/stlink/pull/1076), [#1078](https://github.com/stlink-org/stlink/pull/1078))
- Fixed compilation with GCC 11 ([#1077](https://github.com/stlink-org/stlink/pull/1077))
- [regression] Flash_loader: increased wait rounds for slow boards ([#1085](https://github.com/stlink-org/stlink/pull/1085))
- Fixed support for writing option bytes ([#1102](https://github.com/stlink-org/stlink/pull/1102), [#1128](https://github.com/stlink-org/stlink/pull/1128))
- [doc] Corrected spelling mistake in bug report template ([#1103](https://github.com/stlink-org/stlink/pull/1103))
- Fixed STM32WB55 reading DEBUG IDCODE from the wrong address ([#1100](https://github.com/stlink-org/stlink/pull/1100), [#1101](https://github.com/stlink-org/stlink/pull/1101))
- Applied missing changes to tests ([#1119](https://github.com/stlink-org/stlink/pull/1119))
- Fixed reading of chip ID on Cortex-M0+ core ([#1017](https://github.com/stlink-org/stlink/pull/1017), [#1125](https://github.com/stlink-org/stlink/pull/1125), [#1126](https://github.com/stlink-org/stlink/pull/1126), [#1133](https://github.com/stlink-org/stlink/pull/1133))

# v1.6.1

Release date: 2020-06-01

This release drops support for some older operating systems. Check project README for details.

Features:

- Basic compatibility for STLink/V3 programmer ([#271](https://github.com/stlink-org/stlink/pull/271), [#863](https://github.com/stlink-org/stlink/pull/863), [#954](https://github.com/stlink-org/stlink/pull/954), [#1023](https://github.com/stlink-org/stlink/pull/1023))
  - Added support for JTAG command API v2 & distinguish protocol versions v1 and v2
  - Compatibility with the STLink/V3 firmware which dropped support for the previous API v1
  - As of firmware version J11 the STLink/V1 programmer supports API v2 commands as well
- Display programmer serial when no target is connected ([#432](https://github.com/stlink-org/stlink/pull/432), [#933](https://github.com/stlink-org/stlink/pull/933), [#943](https://github.com/stlink-org/stlink/pull/943))
- Added `connect under reset` to `stlink_open_usb( )` ([#577](https://github.com/stlink-org/stlink/pull/577), [#963](https://github.com/stlink-org/stlink/pull/963))
- Support for STM32L1, SM32L4 option bytes write ([#596](https://github.com/stlink-org/stlink/pull/596), [#844](https://github.com/stlink-org/stlink/pull/844), [#847](https://github.com/stlink-org/stlink/pull/847))
- Added `CMAKEFLAGS` and install target ([#804](https://github.com/stlink-org/stlink/pull/804), [#935](https://github.com/stlink-org/stlink/pull/935))
- Support for STM32G4 ([#822](https://github.com/stlink-org/stlink/pull/822))
- Added aliased SRAM2 region in the L496 memory map ([#824](https://github.com/stlink-org/stlink/pull/824))
- Improved support for STM32G0 ([#825](https://github.com/stlink-org/stlink/pull/825), [#850](https://github.com/stlink-org/stlink/pull/850), [#856](https://github.com/stlink-org/stlink/pull/856), [#857](https://github.com/stlink-org/stlink/pull/857))
- Added postinst script with `depmod -a` for `make package` ([#845](https://github.com/stlink-org/stlink/pull/845), [#931](https://github.com/stlink-org/stlink/pull/931))
- Calculate checksums for flash operations ([#862](https://github.com/stlink-org/stlink/pull/862), [#924](https://github.com/stlink-org/stlink/pull/924))
- Adjust the JTAG/SWD frequency via cmdline option ([#893](https://github.com/stlink-org/stlink/pull/893), [#953](https://github.com/stlink-org/stlink/pull/953))
- Added usb PID and udev rules for STLink/V2.1 found on Nucleo-L432KC and Nucleo-L552ze boards ([#900](https://github.com/stlink-org/stlink/pull/900))
- STM32G0/G4 improvements ([#910](https://github.com/stlink-org/stlink/pull/910))
  - Enable mass erase with a flash programming check
  - Handle G4 Cat3 devices with configurable dual bank flash by using a helper

Updates & changes:

- [doc] Updated compiling instructions ([#113](https://github.com/stlink-org/stlink/pull/113), commit [#10ae529](https://github.com/stlink-org/stlink/commit/10ae5294cd03aacfc07312010f026d3cb12ea56c))
- Defined `libusb` version compatibility for supported systems via `LIBUSB_API_VERSION` ([#211](https://github.com/stlink-org/stlink/pull/211), [#782](https://github.com/stlink-org/stlink/pull/782), [#895](https://github.com/stlink-org/stlink/pull/895))
- Improved argument parsing for CLI tools ([#378](https://github.com/stlink-org/stlink/pull/378), [#922](https://github.com/stlink-org/stlink/pull/922))
- [doc] Updated tutorial: macOS STLink/V1 detection ([#574](https://github.com/stlink-org/stlink/pull/574), [#587](https://github.com/stlink-org/stlink/pull/587))
- Enhanced error log with file path for `map_file()` ([#650](https://github.com/stlink-org/stlink/pull/650), [#879](https://github.com/stlink-org/stlink/pull/879), [#921](https://github.com/stlink-org/stlink/pull/921))
- Enhanced output for error msg `addr not a multiple of pagesize, not supported` ([#663](https://github.com/stlink-org/stlink/pull/663), [#945](https://github.com/stlink-org/stlink/pull/945))
- Updated STLink/V1 driver for macOS ([#735](https://github.com/stlink-org/stlink/pull/735), [#964](https://github.com/stlink-org/stlink/pull/964))
- Package distribution: Provide Windows binaries via Debian-based cross-build ([#738](https://github.com/stlink-org/stlink/pull/738), [#795](https://github.com/stlink-org/stlink/pull/795), [#798](https://github.com/stlink-org/stlink/pull/798), [#870](https://github.com/stlink-org/stlink/pull/870), [#955](https://github.com/stlink-org/stlink/pull/955))
  - [refactoring] Update, corrections & cleanup for build settings (see #955 for details)
  - New `cpack` package-config for DEB and RPM build
  - Update for travis build configuration: builds for `clang -m32`, `clang-9`, MinGW-cross on linux
  - Updated steps for release preparation
  - Project contributors now listed in separate file
  - Test files & gui now use shared `stlink-library`
- [doc] Verify correct udev configuration for device access ([#764](https://github.com/stlink-org/stlink/pull/764))
- Added more error info to `WLOGs` during probe ([#883](https://github.com/stlink-org/stlink/pull/883))
- [doc] Added missing documentation for stlink-gui ([#884](https://github.com/stlink-org/stlink/pull/884))
- Added check for `libssp` during compilation ([#885](https://github.com/stlink-org/stlink/pull/885))
- Silenced unnecessary messages ([#886](https://github.com/stlink-org/stlink/pull/886))
- [doc] Defined `libusb` & `cmake` version compatibility ([#896](https://github.com/stlink-org/stlink/pull/896), [#897](https://github.com/stlink-org/stlink/pull/897), [#899](https://github.com/stlink-org/stlink/pull/899), commit [#27aa888](https://github.com/stlink-org/stlink/commit/27aa88821197d3ffe82baff4e971c3488ec39899))
- Update for STM32G471/473/474/483/484 devices ([#901](https://github.com/stlink-org/stlink/pull/901))
- [doc] `st-flash --flash=n[k][m]` command line option to override device model ([#902](https://github.com/stlink-org/stlink/pull/902))
- [refactoring] Improved cmake build process ([#912](https://github.com/stlink-org/stlink/pull/912))
  - Set up a `libusb` log level accordingly to verbosity ([#894](https://github.com/stlink-org/stlink/pull/894)
  - [compatibility] Updated `libusb` to v1.0.23 ([#895](https://github.com/stlink-org/stlink/pull/895, [#1089](https://github.com/stlink-org/stlink/pull/1089))
  - Updated compiling doc & version support ([#896](https://github.com/stlink-org/stlink/pull/896), [#897](https://github.com/stlink-org/stlink/pull/897), [#899](https://github.com/stlink-org/stlink/pull/899))
  - Version requirements & pkg-maintainer
  - Fixed install paths in build script
  - Updated C-flag `-std=gnu99` to `gnu11`)
  - Added `cmake` uninstall target ([#619](https://github.com/stlink-org/stlink/pull/619), [#907](https://github.com/stlink-org/stlink/pull/907))
  - Integrated module `GNUInstallDirs.cmake` ([#557](https://github.com/stlink-org/stlink/pull/557))
  - [doc] Defined version compatibility and installation instructions for macOS
  - [refactoring] `libusb` detection
  - Deprecated old `appveyor-mingw` script
- [refactoring] BSD-License-compliant rewrite of flashloader source files ([#915](https://github.com/stlink-org/stlink/pull/915), [#932](https://github.com/stlink-org/stlink/pull/932))
- [refactoring] Overall option code rework ([#927](https://github.com/stlink-org/stlink/pull/927))
- [refactoring] Build settings / GUI-Build on UNIX-based systems if `GTK3` is detected ([#929](https://github.com/stlink-org/stlink/pull/929))
- [refactoring] Reconfiguration of package build process ([#931](https://github.com/stlink-org/stlink/pull/931), [#936](https://github.com/stlink-org/stlink/pull/936), [#940](https://github.com/stlink-org/stlink/pull/940), commit [#9b19f92](https://github.com/stlink-org/stlink/commit/9b19f9225460472af9d98959b7217d0a840ee972))
- [refactoring] `st-util`: Removed now useless v1/v2 STLink version stuff ([#934](https://github.com/stlink-org/stlink/pull/934))
- [refactoring] Cleanup for option bytes and flash settings ([#941](https://github.com/stlink-org/stlink/pull/941))
- Added compilation guideline for MSVC toolchain ([#942](https://github.com/stlink-org/stlink/pull/942))
- [refactoring] Cleanup of `cmake` build process ([#944](https://github.com/stlink-org/stlink/pull/944), [#946](https://github.com/stlink-org/stlink/pull/946), [#947](https://github.com/stlink-org/stlink/pull/947))
  - `libusb` package extraction no longer requires `7zip` as an external unarchiver

Fixes:

- Fixed wait-loop for `flash_loader_run()` ([#290](https://github.com/stlink-org/stlink/pull/290))
- Better argument parsing for CLI tools: `stlink_open_usb` can address v1, v2, v3 ([#378](https://github.com/stlink-org/stlink/pull/378), [#922](https://github.com/stlink-org/stlink/pull/922))
- Clear the PG bit before setting the `PER` bit ([#579](https://github.com/stlink-org/stlink/pull/579), [#876](https://github.com/stlink-org/stlink/pull/876))
- Fixed compilation issues with int length on 32-bit platforms ([#629](https://github.com/stlink-org/stlink/pull/629), [#908](https://github.com/stlink-org/stlink/pull/908))
- Fixed `st-info --probe` mechanism ([#679](https://github.com/stlink-org/stlink/pull/679), [#918](https://github.com/stlink-org/stlink/pull/918))
- [regression] Fixed sign-compare (`size != rep_len`) in `usb.c` ([#772](https://github.com/stlink-org/stlink/pull/772), [#869](https://github.com/stlink-org/stlink/pull/869), [#872](https://github.com/stlink-org/stlink/pull/872), [#891](https://github.com/stlink-org/stlink/pull/891))
- Fixed dead loop after an unexpected unplug ([#780](https://github.com/stlink-org/stlink/pull/780), [#812](https://github.com/stlink-org/stlink/pull/812), [#913](https://github.com/stlink-org/stlink/pull/913))
- Avoid re-define of `O_BINARY` on Windows ([#788](https://github.com/stlink-org/stlink/pull/788))
- Fixed stlink lock-up when not connected to a device via JTAG / SWD ([#835](https://github.com/stlink-org/stlink/pull/835), [#943](https://github.com/stlink-org/stlink/pull/943))
- Fixed `st-flash` manpage read example ([#858](https://github.com/stlink-org/stlink/pull/858))
- Fixed stlink support with no mass storage ([#861](https://github.com/stlink-org/stlink/pull/861))
- Make `version.cmake` more error-resistant ([#872](https://github.com/stlink-org/stlink/pull/872))
- Error return in failed probe ([#887](https://github.com/stlink-org/stlink/pull/887), [#890](https://github.com/stlink-org/stlink/pull/890))
- Fixed broken build on 32-bit systems ([#919](https://github.com/stlink-org/stlink/pull/919), [#920](https://github.com/stlink-org/stlink/pull/920))
- `st-flash`: Minor usage fix and make cmdline parsing more user friendly ([#925](https://github.com/stlink-org/stlink/pull/925))
- [regression] Restored functionality of make test builds ([#926](https://github.com/stlink-org/stlink/pull/926), [#929](https://github.com/stlink-org/stlink/pull/929))
- Fixed compilation error due to uninitialized cpuid ([#937](https://github.com/stlink-org/stlink/pull/937), [#938](https://github.com/stlink-org/stlink/pull/938))
- Fixes for STM32F0 flashloader ([#958](https://github.com/stlink-org/stlink/pull/958), [#959](https://github.com/stlink-org/stlink/pull/959))
- Set static link for `libssp` (stack-smashing protection) ([#960](https://github.com/stlink-org/stlink/pull/960), [#961](https://github.com/stlink-org/stlink/pull/961))
- Fixed udev rules installing to wrong directory ([#966](https://github.com/stlink-org/stlink/pull/966))
- Fixed formatting for options display in `st-flash` & `st-info` (commits [#c783d0e](https://github.com/stlink-org/stlink/commit/c783d0e777ccc83a7a8be26a4f4d3414e0478560) and [#562cd24](https://github.com/stlink-org/stlink/commit/562cd2496e696dbd22950925866aac662d81ee5f))

# v1.6.0

Release date: 2020-02-20

Major changes and added features:

- Initial support for STM32L41X ([#754](https://github.com/stlink-org/stlink/pull/754), [#799](https://github.com/stlink-org/stlink/pull/799))
- Verified support for CKS32F103C8T6 and related CKS devices with Core-ID 0x2ba01477 ([#756](https://github.com/stlink-org/stlink/pull/756), [#757](https://github.com/stlink-org/stlink/pull/757), [#805](https://github.com/stlink-org/stlink/pull/805), [#834](https://github.com/stlink-org/stlink/pull/834), Regression-Fixes: [#761](https://github.com/stlink-org/stlink/pull/761), [#766](https://github.com/stlink-org/stlink/pull/766))
- Added preliminary support for some STM32G0 chips ([#759](https://github.com/stlink-org/stlink/pull/759), [#760](https://github.com/stlink-org/stlink/pull/760), [#797](https://github.com/stlink-org/stlink/pull/797))
- Added support for mass erasing second bank on `STM32F10x_XL` ([#767](https://github.com/stlink-org/stlink/pull/767), [#768](https://github.com/stlink-org/stlink/pull/768))
- Added call to clear `PG` bit after writing to flash ([#773](https://github.com/stlink-org/stlink/pull/773))
- Added support to write option bytes for the STM32G0 ([#778](https://github.com/stlink-org/stlink/pull/778))
- Added support for STM32WB55 chips ([#786](https://github.com/stlink-org/stlink/pull/786), [#810](https://github.com/stlink-org/stlink/pull/810), [#816](https://github.com/stlink-org/stlink/pull/816))
- Added `STLink V3SET` VID:PIDs to the udev rules ([#789](https://github.com/stlink-org/stlink/pull/789))
- Support for `STM32+Audio` v2-1 firmware ([#790](https://github.com/stlink-org/stlink/pull/790))
- Build for Windows under Debian/Ubuntu ([#802](https://github.com/stlink-org/stlink/pull/802))
- Allow for 64 bytes serials ([#809](https://github.com/stlink-org/stlink/pull/809))
- Added full support for STLINK CHIP ID L4RX ([#814](https://github.com/stlink-org/stlink/pull/814), [#839](https://github.com/stlink-org/stlink/pull/839))
- Added support for the STLink/V2.1 when flashed with no mass storage (PID 0x3752) ([#819](https://github.com/stlink-org/stlink/pull/819), [#861](https://github.com/stlink-org/stlink/pull/861))
- Added support for writing option bytes on STM32L0xx ([#830](https://github.com/stlink-org/stlink/pull/830))
- Added support to read and write option bytes for STM32F2 series ([#836](https://github.com/stlink-org/stlink/pull/836), [#837](https://github.com/stlink-org/stlink/pull/837))
- Added support to read and write option bytes for STM32F446 ([#843](https://github.com/stlink-org/stlink/pull/843))

Updates and fixes:

- Fixed an issue with versioning stuck at 1.4.0 for versions cloned with git ([#563](https://github.com/stlink-org/stlink/pull/563), [#762](https://github.com/stlink-org/stlink/pull/762), [#772](https://github.com/stlink-org/stlink/pull/772))
- Fixed `unkown chip id`, piped output and `st-util -v` ([#665](https://github.com/stlink-org/stlink/pull/665), [#763](https://github.com/stlink-org/stlink/pull/763))
- Updated STM32F3xx chip ID that covers a few different devices ([#685](https://github.com/stlink-org/stlink/pull/685), [#758](https://github.com/stlink-org/stlink/pull/758))
- Made udev rules and modprobe conf installation optional ([#741](https://github.com/stlink-org/stlink/pull/741))
- Fixed case when `__FILE__` doesn't contain either `/` nor `\\` ([#745](https://github.com/stlink-org/stlink/pull/745))
- Fixed double dash issue in doc/man ([#746](https://github.com/stlink-org/stlink/pull/746), [#747](https://github.com/stlink-org/stlink/pull/747))
- Compiling documentation: package is called `libusb-1.0-0-dev` on Debian ([#748](https://github.com/stlink-org/stlink/pull/748))
- Only do bank calculation on STM32L4 devices with dual banked flash / Added chip-ID 0x464 for STM32L41xxx/L42xxx devices ([#751](https://github.com/stlink-org/stlink/pull/751))
- Added `O_BINARY` option to open file ([#753](https://github.com/stlink-org/stlink/pull/753))
- Fixed versioning when compiling from the checked out git-repo ([#762](https://github.com/stlink-org/stlink/pull/762), [#772](https://github.com/stlink-org/stlink/pull/772))
- win32: move usleep definition to `unistd.h` ([#765](https://github.com/stlink-org/stlink/pull/765))
- Fixed relative path to the UI files needed by `stlink-gui-local` (GUI) ([#770](https://github.com/stlink-org/stlink/pull/770), [#771](https://github.com/stlink-org/stlink/pull/771))
- Added howto for sending `NRST` signal through GDB ([#774](https://github.com/stlink-org/stlink/pull/774), [#776](https://github.com/stlink-org/stlink/pull/776), [#779](https://github.com/stlink-org/stlink/pull/779))
- Fixed package name `devscripts` in doc/compiling.md ([#775](https://github.com/stlink-org/stlink/pull/775))
- Fixed few potential memory/resource leaks ([#803](https://github.com/stlink-org/stlink/pull/803), [#831](https://github.com/stlink-org/stlink/pull/831))
- Updated Linux source repositories in README.md: Debian and Ubuntu ([#821](https://github.com/stlink-org/stlink/pull/821), [#835](https://github.com/stlink-org/stlink/pull/835), [#859](https://github.com/stlink-org/stlink/pull/859))
- Do not issue JTAG reset on STLink/V1 ([#828](https://github.com/stlink-org/stlink/pull/828))
- Fixed flash size of STM32 Discovery VL ([#829](https://github.com/stlink-org/stlink/pull/829))
- Updated documentation on software structure ([#851](https://github.com/stlink-org/stlink/pull/851))

General project updates:

- Updated `README.md`, `CHANGELOG.md` and issue templates (Nightwalker-87)
- Fixed travis build config file (Nightwalker-87)
- Added `CODE_OF_CONDUCT` (Nightwalker-87)
- Archived page from github project wiki to doc/wiki_old.md (Nightwalker-87)

# v1.5.1

Release date: 2018-09-13

Major changes and added features:

- Added reset through `AIRCR` ([#254](https://github.com/stlink-org/stlink/pull/254), [#540](https://github.com/stlink-org/stlink/pull/540), [#712](https://github.com/stlink-org/stlink/pull/712))
- Added creation of icons for `.desktop` file ([#684](https://github.com/stlink-org/stlink/pull/684), [#708](https://github.com/stlink-org/stlink/pull/708))
- Added desktop file for linux ([#688](https://github.com/stlink-org/stlink/pull/688))
- Added button to export STM32 flash memory to a file ([#691](https://github.com/stlink-org/stlink/pull/691))
- Updated `libusb` to 1.0.22 ([#695](https://github.com/stlink-org/stlink/pull/695)) - (related Bugs: [#438](https://github.com/stlink-org/stlink/pull/438), [#632](https://github.com/stlink-org/stlink/pull/632))
- Added icons for stlink GUI ([#697](https://github.com/stlink-org/stlink/pull/697))
- Added support for STM32L4R9 target ([#694](https://github.com/stlink-org/stlink/pull/694), [#699](https://github.com/stlink-org/stlink/pull/699))
- Added memory map for STM32F411RE target ([#709](https://github.com/stlink-org/stlink/pull/709))
- Implemented intel hex support for `GTK` GUI ([#713](https://github.com/stlink-org/stlink/pull/713), [#718](https://github.com/stlink-org/stlink/pull/718))

Updates and fixes:

- Fixed missing flash_loader for STM32L0x ([#269](https://github.com/stlink-org/stlink/pull/269), [#274](https://github.com/stlink-org/stlink/pull/274), [#654](https://github.com/stlink-org/stlink/pull/654), [#675](https://github.com/stlink-org/stlink/pull/675))
- Fix for stlink library calls `exit()` or `_exit()` ([#634](https://github.com/stlink-org/stlink/pull/634), [#696](https://github.com/stlink-org/stlink/pull/696))
- Added semihosting parameter documentation in doc/man ([#674](https://github.com/stlink-org/stlink/pull/674))
- Fixed reference to non-exisiting `st-term` tool in doc/man ([#676](https://github.com/stlink-org/stlink/pull/676))
- Fixed serial number size mismatch with `stlink_open_usb()` ([#680](https://github.com/stlink-org/stlink/pull/680))
- Debian packaging, `cmake` and `README.md` fixes ([#682](https://github.com/stlink-org/stlink/pull/682), [#683](https://github.com/stlink-org/stlink/pull/683))
- Disabled static library installation by default ([#702](https://github.com/stlink-org/stlink/pull/702))
- Fix for `libusb` deprecation ([#703](https://github.com/stlink-org/stlink/pull/703), [#704](https://github.com/stlink-org/stlink/pull/704))
- Renamed `STLINK_CHIPID_STM32_L4R9` to `STLINK_CHIPID_STM32_L4Rx` ([#706](https://github.com/stlink-org/stlink/pull/706))
- [regression] stlink installation under Linux (Debian 9) is broken since #695 ([#700](https://github.com/stlink-org/stlink/pull/700), [#701](https://github.com/stlink-org/stlink/pull/701), [#707](https://github.com/stlink-org/stlink/pull/707))
- Fixed flash memory map for STM32F72xxx target ([#711](https://github.com/stlink-org/stlink/pull/711))
- Proper flash page size calculation for STM32F412xx target ([#721](https://github.com/stlink-org/stlink/pull/721))
- Return correct value on `EOF` for semihosting `SYS_READ` ([#726](https://github.com/stlink-org/stlink/pull/726), [#727](https://github.com/stlink-org/stlink/pull/727), [#728](https://github.com/stlink-org/stlink/pull/728), [#729](https://github.com/stlink-org/stlink/pull/729), [#730](https://github.com/stlink-org/stlink/pull/730), [#731](https://github.com/stlink-org/stlink/pull/731), [#732](https://github.com/stlink-org/stlink/pull/732))
- FreeBSD defines `LIBUSB_API_VERSION` instead of `LIBUSBX_API_VERSION` ([#733](https://github.com/stlink-org/stlink/pull/733))

# v1.5.0

Release date: 2018-02-16

Major changes and added features:

- Added support of STM32L496xx/4A6xx devices ([#615](https://github.com/stlink-org/stlink/pull/615), [#657](https://github.com/stlink-org/stlink/pull/657))
- Added unknown chip dummy to obtain the serial of the STlink by a call to `st-info --probe` ([#641](https://github.com/stlink-org/stlink/pull/641))
- Added support for STM32F72xx (chip-ID: 0x452) devices (commit [#1969148](https://github.com/stlink-org/stlink/commit/19691485359afef1a256964afcbb8dcf4b733209))

Updates and fixes:

- Fixed verification of flash error for STM32L496x device ([#617](https://github.com/stlink-org/stlink/pull/617), [#618](https://github.com/stlink-org/stlink/pull/618))
- Updated Linux source repositories in README.md: Gentoo, Fedora and RedHat/CentOS ([#622](https://github.com/stlink-org/stlink/pull/622), [#635](https://github.com/stlink-org/stlink/pull/635))
- Updated changelog in debian package ([#630](https://github.com/stlink-org/stlink/pull/630))
- Added `LIB_INSTALL_DIR` to correct libs install on 64-bit systems ([#633](https://github.com/stlink-org/stlink/pull/633), [#636](https://github.com/stlink-org/stlink/pull/636))
- Fixed write for microcontroller with RAM size less or equal to 32K ([#637](https://github.com/stlink-org/stlink/pull/637))
- Fixed memory map for STM32L496xx boards ([#639](https://github.com/stlink-org/stlink/pull/639))
- Fixed `__FILE__` base name extraction ([#624](https://github.com/stlink-org/stlink/pull/624), [#628](https://github.com/stlink-org/stlink/pull/628), [#648](https://github.com/stlink-org/stlink/pull/648))
- Added debian/triggers to run `ldconfig` ([#664](https://github.com/stlink-org/stlink/pull/664))
- Fixed build on Fedora with GCC 8 ([#666](https://github.com/stlink-org/stlink/pull/666), [#667](https://github.com/stlink-org/stlink/pull/667), [#668](https://github.com/stlink-org/stlink/pull/668))

# v1.4.0

Release date: 2017-07-01

Major changes and added features:

- Allow building of debian package with CPack ([#554](https://github.com/stlink-org/stlink/pull/554), commit [#5b69f25](https://github.com/stlink-org/stlink/commit/5b69f25198a1a8f34e2ee48d1ad20f79447e3d55))
- Added support for STM32L011 target ([#564](https://github.com/stlink-org/stlink/pull/564), [#565](https://github.com/stlink-org/stlink/pull/565), [#572](https://github.com/stlink-org/stlink/pull/572))
- Added support for flashing second bank on STM32F10x_XL ([#592](https://github.com/stlink-org/stlink/pull/592))
- Initial support to compile with Microsoft Visual Studio 2017 ([#602](https://github.com/stlink-org/stlink/pull/602))
- Added support for STM32L452 target ([#603](https://github.com/stlink-org/stlink/pull/603), [#608](https://github.com/stlink-org/stlink/pull/608))

Updates and fixes:

- Added `--flash=n[k][m]` command line option to override device model ([#305](https://github.com/stlink-org/stlink/pull/305), [#516](https://github.com/stlink-org/stlink/pull/516), [#576](https://github.com/stlink-org/stlink/pull/576))
- Updated `libusb` to 1.0.21 for Windows ([#562](https://github.com/stlink-org/stlink/pull/562))
- Fixed low-voltage flashing on STM32F7 devices ([#566](https://github.com/stlink-org/stlink/pull/566), [#567](https://github.com/stlink-org/stlink/pull/567))
- Fixed building with mingw64 ([#569](https://github.com/stlink-org/stlink/pull/569), [#573](https://github.com/stlink-org/stlink/pull/573), [#578](https://github.com/stlink-org/stlink/pull/578), [#582](https://github.com/stlink-org/stlink/pull/582), [#584](https://github.com/stlink-org/stlink/pull/584), [#610](https://github.com/stlink-org/stlink/pull/610), [#846](https://github.com/stlink-org/stlink/pull/846))
- Fixed possible memory leak ([#570](https://github.com/stlink-org/stlink/pull/570), [#571](https://github.com/stlink-org/stlink/pull/571))
- Fixed installation path for shared objects ([#581](https://github.com/stlink-org/stlink/pull/581))
- Fixed a few `-Wformat` warnings ([#582](https://github.com/stlink-org/stlink/pull/582))
- Removed unused defines in `mingw.h` ([#583](https://github.com/stlink-org/stlink/pull/583))
- Skip `GTK` detection when cross-compiling ([#588](https://github.com/stlink-org/stlink/pull/588))
- Fixed compilation with GCC 7 ([#590](https://github.com/stlink-org/stlink/pull/590), [#591](https://github.com/stlink-org/stlink/pull/591))
- Fixed flashing to `F0 device` targets ([#594](https://github.com/stlink-org/stlink/pull/594), [#595](https://github.com/stlink-org/stlink/pull/595))
- Fixed wrong counting when flashing ([#605](https://github.com/stlink-org/stlink/pull/605))

# v1.3.1

Release date: 2017-02-25

Major changes and added features:

- Added support for Semihosting `SYS_READC` ([#546](https://github.com/stlink-org/stlink/pull/546))
- Added support for STM32F413 ([#549](https://github.com/stlink-org/stlink/pull/549), [#550](https://github.com/stlink-org/stlink/pull/550), [#758](https://github.com/stlink-org/stlink/pull/758))
- Added preliminary support for STM32L011 to see it after probe (chip-ID 0x457) ([#558](https://github.com/stlink-org/stlink/pull/558), [#598](https://github.com/stlink-org/stlink/pull/598))

Updates and fixes:

- `cmake/CPackConfig.cmake`: Fixup OSX zip filename
- Updated source repositories in README.md: Windows, macOS, Alpine Linux
- Compilation fixes ([#547](https://github.com/stlink-org/stlink/pull/547), [#551](https://github.com/stlink-org/stlink/pull/551), [#552](https://github.com/stlink-org/stlink/pull/552))
- Stripped full paths to source files in log ([#548](https://github.com/stlink-org/stlink/pull/548))
- Fixed incorrect release folder name in docs ([#560](https://github.com/stlink-org/stlink/pull/560))
- Fixed compilation when path includes spaces ([#561](https://github.com/stlink-org/stlink/pull/561))

# v1.3.0

Release date: 2017-01-28

Major changes and added features:

- Deprecation of autotools (`autoconf`, `automake`) and fixed build with MinGW ([#83](https://github.com/stlink-org/stlink/pull/83), [#431](https://github.com/stlink-org/stlink/pull/431), [#434](https://github.com/stlink-org/stlink/pull/434), [#465](https://github.com/stlink-org/stlink/pull/465))
- Added intel hex file reading for `st-flash` ([#110](https://github.com/stlink-org/stlink/pull/110), [#157](https://github.com/stlink-org/stlink/pull/157), [#457](https://github.com/stlink-org/stlink/pull/547), [#459](https://github.com/stlink-org/stlink/pull/549))
- Added support for ARM semihosting to `st-util` ([#147](https://github.com/stlink-org/stlink/pull/147), [#227](https://github.com/stlink-org/stlink/pull/227), [#454](https://github.com/stlink-org/stlink/pull/454), [#455](https://github.com/stlink-org/stlink/pull/455))
- Added manpages (generated with `pandoc` from Markdown) ([#208](https://github.com/stlink-org/stlink/pull/208), [#464](https://github.com/stlink-org/stlink/pull/464), [#466](https://github.com/stlink-org/stlink/pull/466), [#467](https://github.com/stlink-org/stlink/pull/467))
- Removal of undocumented `st-term` utility, which is now replaced by `st-util` ARM semihosting feature ([#228](https://github.com/stlink-org/stlink/pull/228), [#507](https://github.com/stlink-org/stlink/pull/507), commit [#3fd0f09](https://github.com/stlink-org/stlink/commit/3fd0f099782506532198473b24f643a3f68d5ff9))
- Support serial numbers argument for `st-util` and `st-flash` to probe and control multiple connected programmers ([#318](https://github.com/stlink-org/stlink/pull/318), [#398](https://github.com/stlink-org/stlink/pull/398), [#541](https://github.com/stlink-org/stlink/pull/541))
- Added 'k' (kill) command to gdb-server, which resets the connection ([#358](https://github.com/stlink-org/stlink/pull/358), [#525](https://github.com/stlink-org/stlink/pull/525), [#527](https://github.com/stlink-org/stlink/pull/527), [#528](https://github.com/stlink-org/stlink/pull/528))
- Merge `st-probe` tool into `st-info` ([#398](https://github.com/stlink-org/stlink/pull/398))
- Added support for native debian packaging ([#444](https://github.com/stlink-org/stlink/pull/444), [#472](https://github.com/stlink-org/stlink/pull/472), [#473](https://github.com/stlink-org/stlink/pull/473), [#482](https://github.com/stlink-org/stlink/pull/482), [#483](https://github.com/stlink-org/stlink/pull/483), [#484](https://github.com/stlink-org/stlink/pull/484), [#485](https://github.com/stlink-org/stlink/pull/485))
- Rewritten commandline parsing for `st-flash` ([#459](https://github.com/stlink-org/stlink/pull/459))
- Added `--reset` command to `st-flash` ([#505](https://github.com/stlink-org/stlink/pull/505))

Chip support added for:

- STM32F401XE: Added memory map for device ([#460](https://github.com/stlink-org/stlink/pull/460))
- STM32F410RBTx ([#418](https://github.com/stlink-org/stlink/pull/418))
- STM32F412 ([#537](https://github.com/stlink-org/stlink/pull/537), [#538](https://github.com/stlink-org/stlink/pull/538))
- STM32F7xx ([#324](https://github.com/stlink-org/stlink/pull/324), [#326](https://github.com/stlink-org/stlink/pull/326), [#327](https://github.com/stlink-org/stlink/pull/327), [#337](https://github.com/stlink-org/stlink/pull/337))
- STM32F7x7x ([#433](https://github.com/stlink-org/stlink/pull/433), [#435](https://github.com/stlink-org/stlink/pull/435), [#436](https://github.com/stlink-org/stlink/pull/436), [#509](https://github.com/stlink-org/stlink/pull/509))
- STM32L0xx Cat2 devices (chip-ID: 0x425) ([#414](https://github.com/stlink-org/stlink/pull/414))
- STM32L0xx Cat5 devices (chip-ID: 0x447) ([#387](https://github.com/stlink-org/stlink/pull/387), [#406](https://github.com/stlink-org/stlink/pull/406))
- STM32L4xx ([#321](https://github.com/stlink-org/stlink/pull/321))
- STM32L432 ([#500](https://github.com/stlink-org/stlink/pull/500), [#501](https://github.com/stlink-org/stlink/pull/501))

Updates and fixes:

- Do a JTAG reset prior to reading CPU information when processor is in deep sleep ([#291](https://github.com/stlink-org/stlink/pull/291), [#428](https://github.com/stlink-org/stlink/pull/428), [#430](https://github.com/stlink-org/stlink/pull/430), [#451](https://github.com/stlink-org/stlink/pull/451))
- Fixed `unaligned addr or size` when trying to write a program in RAM ([#323](https://github.com/stlink-org/stlink/pull/323))
- Fixed flashing on `STM32_F3_SMALL` ([#325](https://github.com/stlink-org/stlink/pull/325))
- Fixed STM32L-problem with flash loader ([#390](https://github.com/stlink-org/stlink/pull/390), [#407](https://github.com/stlink-org/stlink/pull/407), [#408](https://github.com/stlink-org/stlink/pull/408))
- Don't read the target voltage on startup, because it crashes STM32F100 ([#423](https://github.com/stlink-org/stlink/pull/423), [#424](https://github.com/stlink-org/stlink/pull/424))
- Added a useful error message instead of `[!] send_recv` ([#425](https://github.com/stlink-org/stlink/pull/425), [#426](https://github.com/stlink-org/stlink/pull/426))
- Fixed STM32F030 erase error ([#442](https://github.com/stlink-org/stlink/pull/442))
- Fixed memory map for STM32F7xx ([#453](https://github.com/stlink-org/stlink/pull/453), [#456](https://github.com/stlink-org/stlink/pull/456))
- Redesign of `st-flash` commandline options parsing ([#459](https://github.com/stlink-org/stlink/pull/459))
- Set `SWDCLK` and fixed `jtag_reset` bug ([#462](https://github.com/stlink-org/stlink/pull/462), [#475](https://github.com/stlink-org/stlink/pull/475), [#534](https://github.com/stlink-org/stlink/pull/534))
- doc/compiling.md: Add note about installation and `ldconfig` ([#478](https://github.com/stlink-org/stlink/pull/478), commit [#be66bbf](https://github.com/stlink-org/stlink/commit/be66bbf200c718904514b044ba84d64a36456218))
- Fixed Release target to generate the man-pages with `pandoc` ([#479](https://github.com/stlink-org/stlink/pull/479))
- Fixed Cygwin build ([#487](https://github.com/stlink-org/stlink/pull/487), ([#506](https://github.com/stlink-org/stlink/pull/506))
- Reset flash mass erase (MER) bit after mass erase for safety ([#489](https://github.com/stlink-org/stlink/pull/489))
- Wrong extract command in `FindLibUSB.cmake` ([#510](https://github.com/stlink-org/stlink/pull/510), [#511](https://github.com/stlink-org/stlink/pull/511))
- Fixed compilation error on Ubuntu 16.10 ([#514](https://github.com/stlink-org/stlink/pull/514), [#525](https://github.com/stlink-org/stlink/pull/525))

# v1.2.0

Release date: 2016-05-16

Features added:

- Added multiple stlink probing (`st-info --probe`, `st-info --hla-serial`) with printing serial in hex and OpenOCD `hla_serial` format (Jerry Jacobs)
- Added stlink usb probe API functions (Jerry Jacobs)
- Added parameter to specify one stlink v2 of many (Georg von Zengen)

Updates and fixes:

- Refactoring/fixes of flash loader (Maxime Coquelin)
- Synchronized cache for STM32F7 (Tristan Gingold)
- Allow flashing of STM32L4 down to 1.71 V (Greg Meiste)
- Fix on STM32L4 to clear flash mass erase flags on CR (Bruno Dal Bo)
- Proper writing of page 0 of second bank for STM32L476xe (Tobias Badertscher)
- Trace the read data in `stlink_read_debug32` and not the address of the variable (Tobias Badertscher)
- Mac OS X El Capitan platform support confirmation (Nikolay)
- Do not send a `NULL` at end of packets to `gdb` (Tristan Gingold)
- Correctly compute flash write size for partial pages (Dave Vandervies)
- `_stlink_usb_reset` use hardreset (mlundinse)
- Make sure MCU is halted before running RAM based flashloaders (mlundinse)
- Could not flash `STM32_F3_SMALL` (Max Chen)
- STM32F4 8-bit support for 1.8v operation (Andy Isaacson)
- Fixed STM32F2xx memory map (Nicolas Schodet)
- Memory map for STM32F42xxx and STM32F43xxx devices (Craig Lilley)
- Stm32l0x flash loader (Robin Kreis)
- Modified determination of erased byte pattern when flashing ([#193](https://github.com/stlink-org/stlink/pull/193), [#377](https://github.com/stlink-org/stlink/pull/377))
- Use libusb synchronous api ([#194](https://github.com/stlink-org/stlink/pull/194), [#225](https://github.com/stlink-org/stlink/pull/225), [#374](https://github.com/stlink-org/stlink/pull/374))
- Fixed segfault when programmer is already busy and `NULL` pointers are in the list ([#256](https://github.com/stlink-org/stlink/pull/256), [#394](https://github.com/stlink-org/stlink/pull/394))
- Fixed gdb-server: Cortex M0 chips have no `FP_CTRL` register for breakpoints ([#266](https://github.com/stlink-org/stlink/pull/266), [#273](https://github.com/stlink-org/stlink/pull/273), [#341](https://github.com/stlink-org/stlink/pull/341))
- Fixed issue where "unknown chip id!" was seen every other time ([#352](https://github.com/stlink-org/stlink/pull/352), [#367](https://github.com/stlink-org/stlink/pull/367), [#381](https://github.com/stlink-org/stlink/pull/381))
- Send F4 memory-map and features for STM32F429 ([#188](https://github.com/stlink-org/stlink/pull/188), [#196](https://github.com/stlink-org/stlink/pull/196), [#250](https://github.com/stlink-org/stlink/pull/250), [#251](https://github.com/stlink-org/stlink/pull/251)) (Release v1.1.0)
- Added AHB3 Peripherals definition for STM32F4 ([#218](https://github.com/stlink-org/stlink/pull/218), [#288](https://github.com/stlink-org/stlink/pull/288)) (Release v1.1.0)
- Reset: st-flash does not work when CPU is in sleep mode ([#62](https://github.com/stlink-org/stlink/pull/62)) (Release v1.0.0)
- Ensure USB device search succeeds if the matched device is at index 0 ([#126](https://github.com/stlink-org/stlink/pull/126), [#151](https://github.com/stlink-org/stlink/pull/151)) (Release v1.0.0)
- Corrected flash size register address for STM32F2 devices ([#278](https://github.com/stlink-org/stlink/pull/278)) (Release v1.0.0)

Chip support added for:

- STM32L053R8 (Jean-Luc Béchennec)
- STM32F7 Support (mlundinse)
- Added STM32L4 to CHIPID #defines and devices[], flash driver and loader (Dave Vandervies)
- Basic support for STM32F446 (Pavel Kirienko)
- STM32F303 High Density
- STM32F469/STM32F479 ([#345](https://github.com/stlink-org/stlink/pull/345), [#555](https://github.com/stlink-org/stlink/pull/555)) (Release v1.2.0)
- STM32L1xx Cat.2 devices (Nicolas Schodet)
- STM32L1xx (chip-ID 0x427) ([#152](https://github.com/stlink-org/stlink/pull/152), [#163](https://github.com/stlink-org/stlink/pull/163), [#165](https://github.com/stlink-org/stlink/pull/165)) (Release v1.0.0)
- Added `SIGINT` handler for stlink cleanup ([#31](https://github.com/stlink-org/stlink/pull/31), [#135](https://github.com/stlink-org/stlink/pull/135)) (Release v1.0.0)

Board support added for:

- Nucleo-F303RE (Kyle Manna)
- Nucleo-F411RE (texane)

Build system:

- Travis: Initial support for Travis continues integration on Linux & Mac OS X (Jerry Jacobs)
- CMake: Document in README.md and add extra strict compiler flags (Jerry Jacobs)
- CMake: First stab at a `cmake` build (Josh Bialkowski)
