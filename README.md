Open source version of the STMicroelectronics STlink Tools
==========================================================

[![BSD licensed](https://img.shields.io/badge/license-BSD-blue.svg)](https://raw.githubusercontent.com/hyperium/hyper/master/LICENSE)
[![GitHub release](https://img.shields.io/github/release/texane/stlink.svg)](https://github.com/stlink-org/stlink/releases/latest)
[![GitHub commits](https://img.shields.io/github/commits-since/texane/stlink/v1.6.0.svg)](https://github.com/stlink-org/stlink/releases/master)
[![Downloads](https://img.shields.io/github/downloads/texane/stlink/total.svg)](https://github.com/stlink-org/stlink/releases)
[![Linux Status](https://img.shields.io/travis/texane/stlink/master.svg?label=linux)](https://travis-ci.org/stlink-org/stlink)
[![macOS Status](https://img.shields.io/travis/texane/stlink/master.svg?label=osx)](https://travis-ci.org/stlink-org/stlink)

Recent new features and bugfixes can be found in the [Changelog](CHANGELOG.md) of this software project.


#### License

The stlink library and tools are licensed under the **[BSD-3 License](LICENSE.md)**.<br />
The source files **stm32l0x.s** and **stm32lx.s** found in the subdirectory `/flashloaders/`
are licensed under the **General Public License (GPL v2+)**.


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

Currently known working combinations of programmers and targets are listed in [devices_boards.md](doc/devices_boards.md).


## Tutorial & HOWTO

Our [tutorial.md](doc/tutorial.md may help you along with some advanced tasks and additional info.


## Installation

**Windows**:

Please compile and install from source as described in our [compiling manual](doc/compiling.md#Windows).

Long awaited binaries will be available soon...

**macOS**:

We recommend to install from:

* [homebrew](https://formulae.brew.sh/formula/stlink) or
* [MacPorts](https://ports.macports.org/port/stlink)

Alternatively one can compile and install from source as described in our [compiling manual](doc/compiling.md#macOS).

**Linux**:

We recommend to install `stlink-tools` from the package repository of the used distribution:

* Debian Linux: [(Link)](https://packages.debian.org/buster/stlink-tools)
* Ubuntu Linux: [(Link)](https://packages.ubuntu.com/stlink-tools)
* Arch Linux:   [(Link)](https://www.archlinux.org/packages/community/x86_64/stlink)
* Alpine Linux: [(Link)](https://pkgs.alpinelinux.org/packages?name=stlink)
* Fedora:       [(Link)](https://src.fedoraproject.org/rpms/stlink)
* Gentoo Linux: [(Link)](https://packages.gentoo.org/packages/dev-embedded/stlink)

**Other Operating Systems**:

* RedHat/CentOS 8: Users can install from [EPEL repository](https://src.fedoraproject.org/rpms/stlink/branch/epel8)
* FreeBSD: Users can install from [freshports](https://www.freshports.org/devel/stlink)
* OpenBSD: Users need to compile and install from source as described in our [compiling manual](doc/compiling.md).


## Installation from source (advanced users)

When there is no executable available for your platform or you need the latest (possible unstable) version you need to compile the toolset yourself. This procedure is explained in the [compiling manual](doc/compiling.md).


## Contributing and versioning

* The semantic versioning scheme is used. Read more at [semver.org](http://semver.org)
* Before creating a pull request, please _ALWAYS_ open a new issue for the discussion of the intended new features. Bugfixes don't require a discussion via a ticket-issue. However they should always be described in a few words as soon as they appear to help others as well.
* Contributors and/or maintainers may submit comments or request changes to patch-proposals and/or pull-requests.
* **ATTENTION: _NEVER EVER_ use the '#' character to count-up single points within a listing as '#' is _exclusively_ reserved for referencing github issues and pull-requests. Otherwise you accidentally introduce false cross references within the project.**
* Please start new forks from the develop branch if possible as pull requests will go into this branch as well.


# Current state of the project
## Known missing features

Some features are currently missing from the `texane/stlink` toolset.
Here we would appreciate any help and would love to welcome new contributors who want to get involved:

* Instrumentation Trace Macro (ITM) Cell ([#136](https://github.com/texane/stlink/issues/136))
* OTP & EEPROM area programming ([#202](https://github.com/texane/stlink/issues/202), [#333](https://github.com/texane/stlink/issues/333), [#686](https://github.com/texane/stlink/issues/686))
* Protection bits area reading ([#346](https://github.com/texane/stlink/issues/346))
* Writing external memory connected to an STM32 controller (e.g Quad SPI NOR flash) ([#412](https://github.com/texane/stlink/issues/412))
* MCU hotplug ([#449](https://github.com/texane/stlink/issues/449))
* Writing options bytes (region) ([#458](https://github.com/texane/stlink/issues/458))
* Support for STLINKv3 programmer ([#820](https://github.com/texane/stlink/issues/820))
