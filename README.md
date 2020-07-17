Open source version of the STMicroelectronics STlink Tools
==========================================================

[![BSD licensed](https://img.shields.io/badge/license-BSD-blue.svg)](https://raw.githubusercontent.com/hyperium/hyper/master/LICENSE)
[![GitHub release](https://img.shields.io/github/release/stlink-org/stlink.svg)](https://github.com/stlink-org/stlink/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/stlink-org/stlink/total)](https://github.com/stlink-org/stlink/releases/latest)
![GitHub commits](https://img.shields.io/github/commits-since/stlink-org/stlink/v1.6.1/develop)
![GitHub activity](https://img.shields.io/github/commit-activity/m/stlink-org/stlink)
![GitHub contributors](https://img.shields.io/github/contributors/stlink-org/stlink)
[![Linux Status](https://img.shields.io/travis/stlink-org/stlink/master?env=BADGE=linux&label=linux)](https://travis-ci.org/stlink-org/stlink)
[![macOS Status](https://img.shields.io/travis/stlink-org/stlink/master?env=BADGE=osx&label=osx)](https://travis-ci.org/stlink-org/stlink)

Recent new features and bugfixes can be found in the [Changelog](CHANGELOG.md) of this software project.


#### License

The stlink library and tools are licensed under the **[BSD-3 License](LICENSE.md)**.


## Introduction

STLink is an open source toolset to program and debug STM32 devices and boards manufactured by STMicroelectronics.
It supports several so called STLINK programmer boards (and clones thereof) which use a microcontroller chip to translate commands from USB to JTAG/SWD. There are four generations available on the market which are _all_ supported by this toolset:

* **STLINK/v1** _(obsolete as of 21-11-2019, continued support by this toolset)_
  - transport layer: SCSI passthru commands over USB
  - stand-alone programmer and present on STM32VL Discovery boards
* **STLINK/v2**
  - transport layer: raw USB commands
  - stand-alone programmer and present on STM32L Discovery and Nucleo boards
* **STLINK/v2-1**
  - transport layer: raw USB commands
  - present on some STM32 Nucleo boards
* **STLINK/v3**
  - transport layer: raw USB commands
  - stand-alone programmer

On the user level there is no difference in handling or operation between these different revisions.

The STlink toolset includes:

* a communication library (libstlink.a),
* a programmer and chip information tool (st-info),
* a flash manipulation tool (st-flash),
* a GDB server (st-util) and
* a GUI-Interface (stlink-gui) _[optional]_


## Supported operating systems and hardware combinations

Currently known working combinations of programmers and targets are listed in [devices_boards.md](doc/devices_boards.md).

Supported operating systems are listed in [version_support.md](doc/version_support.md).

The `stlink` toolset continues to maintain backwards compatibility with the **STLINK/v1** programmer.<br />
Please note that on macOS this support is limited to versions 10.13 - 10.15.

## Tutorial & HOWTO

Our [tutorial](doc/tutorial.md) may help you along with some advanced tasks and additional info.


## Installation

**Windows**:

As of Release v1.6.1 stand-alone Windows binaries are made available (again) on the release page of the project.
Please ensure to select the correct version for your system (i686 or x86_64). The archive file can be unzipped to any desired location as it does not contain any hardcoded paths. However we suggest to move the unzipped application folder to `C:\Program Files\` on 32-bit systems and to `C:\Program Files (x86)\` on 64-bit systems (the toolset is a 32-bit).

Alternatively one may compile and install from source as described in our [compiling manual](doc/compiling.md#Windows).

**macOS**:

We recommend to install from:

* [homebrew](https://formulae.brew.sh/formula/stlink) or
* [MacPorts](https://ports.macports.org/port/stlink)

Alternatively one can compile and install from source as described in our [compiling manual](doc/compiling.md#macOS).

**Linux**:

We recommend to install `stlink-tools` from the package repository of the used distribution:

**Note:** As packages distributed via the [Debian](https://packages.debian.org/buster/stlink-tools) and [Ubuntu](https://packages.ubuntu.com/stlink-tools) repositories differ from our self-maintained deb-package, we recommend to use the latter instead (see link below). It provides the opportunity to handle and fix user-reported package issues directly within the project and is not redundant to any limitations deriving from external maintenance guidelines.

* Debian Linux: [(Link)](https://github.com/stlink-org/stlink/releases)
* Ubuntu Linux: [(Link)](https://github.com/stlink-org/stlink/releases)
* Arch Linux:   [(Link)](https://www.archlinux.org/packages/community/x86_64/stlink)
* Alpine Linux: [(Link)](https://pkgs.alpinelinux.org/packages?name=stlink)
* Fedora:       [(Link)](https://src.fedoraproject.org/rpms/stlink)
* Gentoo Linux: [(Link)](https://packages.gentoo.org/packages/dev-embedded/stlink)

**Other Operating Systems**:

* RedHat/CentOS 8: Users can install from [EPEL repository](https://src.fedoraproject.org/rpms/stlink/branch/epel8)
* FreeBSD: Users can install from [freshports](https://www.freshports.org/devel/stlink)


## Installation from source (advanced users)

When there is no executable available for your platform or you need the latest (possible unstable) version you need to compile the toolset yourself. This procedure is explained in the [compiling manual](doc/compiling.md).


## Contributing and versioning

* The semantic versioning scheme is used. Read more at [semver.org](http://semver.org)
* Before creating a pull request, please _ALWAYS_ open a new issue for the discussion of the intended new features. Bugfixes don't require a discussion via a ticket-issue. However they should always be described in a few words as soon as they appear to help others as well.
* Contributors and/or maintainers may submit comments or request changes to patch-proposals and/or pull-requests.
* **ATTENTION: _NEVER EVER_ use the '#' character to count-up single points within a listing as '#' is _exclusively_ reserved for referencing GitHub issues and pull-requests. Otherwise you accidentally introduce false cross references within the project.**
* Please start new forks from the develop branch, as pull requests will go into this branch as well.

Please also refer to our [Contribution Guidelines](CONTRIBUTING.md).


# Current state of the project
## Known missing features

Some features are currently missing from the `stlink-org/stlink` toolset.
Here we would appreciate any help and would love to welcome new contributors who want to get involved:

* Instrumentation Trace Macro (ITM) Cell ([#136](https://github.com/stlink-org/stlink/issues/136))
* OTP & EEPROM area programming ([#202](https://github.com/stlink-org/stlink/issues/202), [#333](https://github.com/stlink-org/stlink/issues/333), [#686](https://github.com/stlink-org/stlink/issues/686))
* Protection bits area reading ([#346](https://github.com/stlink-org/stlink/issues/346))
* Writing external memory connected to an STM32 controller (e.g Quad SPI NOR flash) ([#412](https://github.com/stlink-org/stlink/issues/412))
* MCU hotplug ([#449](https://github.com/stlink-org/stlink/issues/449))
* Writing options bytes (region) ([#458](https://github.com/stlink-org/stlink/issues/458))
* Enhanced support for STLINKv3 programmer ([#820](https://github.com/stlink-org/stlink/issues/820))
