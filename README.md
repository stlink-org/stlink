# Open source version of the STMicroelectronics STLINK Tools

[![BSD licensed](https://img.shields.io/badge/license-BSD-blue.svg)](https://raw.githubusercontent.com/hyperium/hyper/master/LICENSE)
[![GitHub release](https://img.shields.io/github/release/stlink-org/stlink.svg)](https://github.com/stlink-org/stlink/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/stlink-org/stlink/total)](https://github.com/stlink-org/stlink/releases/latest)
![GitHub commits](https://img.shields.io/github/commits-since/stlink-org/stlink/v1.7.0/develop)
![GitHub activity](https://img.shields.io/github/commit-activity/m/stlink-org/stlink)
![GitHub contributors](https://img.shields.io/github/contributors/stlink-org/stlink)
[![CodeQL](https://github.com/stlink-org/stlink/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/stlink-org/stlink/actions/workflows/codeql-analysis.yml)
[![C/C++ CI](https://github.com/stlink-org/stlink/actions/workflows/c-cpp.yml/badge.svg?branch=testing)](https://github.com/stlink-org/stlink/actions/workflows/c-cpp.yml)
[![Linux Status](https://img.shields.io/travis/stlink-org/stlink/master?env=BADGE=linux&label=linux)](https://travis-ci.org/stlink-org/stlink)
[![macOS Status](https://img.shields.io/travis/stlink-org/stlink/master?env=BADGE=osx&label=osx)](https://travis-ci.org/stlink-org/stlink)

Recent new features and bugfixes can be found in the [Changelog](CHANGELOG.md) of this software project.

#### License

The stlink library and tools are licensed under the **[BSD-3 License](LICENSE.md)**.

## Introduction

stlink is an open source toolset to program and debug STM32 devices and boards manufactured by STMicroelectronics.
It supports several so called STLINK programmer boards (and clones thereof) which use a microcontroller chip to translate commands from USB to JTAG/SWD. There are four generations available on the market which are _all_ supported by this toolset:

- **STLINK/V1** _[obsolete as of 21-11-2019, continued support by this toolset] \*)_
  - transport layer: SCSI passthru commands over USB
  - stand-alone programmer
  - on-board on STM32VL Discovery boards
- **STLINK/V2**
  - transport layer: raw USB commands
  - stand-alone programmer
  - on-board on STM32L Discovery and STM32 Nucleo boards
- **STLINK/V2-1**
  - transport layer: raw USB commands
  - on-board on some STM32 Nucleo boards
- **STLINK-V3**
  - transport layer: raw USB commands
  - stand-alone programmer (STLINK-V3SET, STLINK-V3MINI, STLINK-V3MODS)
  - on-board on some STM32 Nucleo boards (STLINK-V3E)

_\*)_ *Note: Support for the STLINK/V1 on macOS is limited to 10.15. Due to the deprecation and removal of macOS Kernel Extensions (KEXT) there will be no support for this programmer on macOS 11 or any later version.*

On the user level there is no difference in handling or operation between these different revisions.

The STlink toolset includes:

- `st-info` - a programmer and chip information tool
- `st-flash` - a flash manipulation tool
- `st-trace` - a logging tool to record information on execution
- `st-util` - a GDB server (supported in Visual Studio Code / VSCodium via the [Cortex-Debug](https://github.com/Marus/cortex-debug) plugin)
- `stlink-lib` - a communication library
- `stlink-gui` - a GUI-Interface _[optional]_

## Supported operating systems and hardware combinations

Currently known working MCU targets are listed in [devices_boards.md](doc/devices_boards.md).

A list of supported operating can be found in [version_support.md](doc/version_support.md).

## Tutorial & HOWTO

Our [tutorial](doc/tutorial.md) may help you along with some advanced tasks and additional info.

## Installation

**Windows**:

As of Release v1.6.1 stand-alone Windows binaries are made available (again) on the release page of the project.
Please ensure to select the correct version for your system (i686 or x86_64). The archive file can be unzipped to any desired location as it does not contain any hardcoded paths. However we suggest to move the unzipped application folder to `C:\Program Files\` on 32-bit systems and to `C:\Program Files (x86)\` on 64-bit systems (the toolset is 32-bit).

Alternatively one may compile and install from source as described in our [compiling manual](doc/compiling.md#Windows).

**macOS**:

We recommend to install from:

- [homebrew](https://formulae.brew.sh/formula/stlink) or
- [MacPorts](https://ports.macports.org/port/stlink)

Alternatively one can compile and install from source as described in our [compiling manual](doc/compiling.md#macOS).

**Linux**:

We recommend to install `stlink-tools` from the package repository of the used distribution:

**Note:** As packages distributed via the [Debian](https://packages.debian.org/buster/stlink-tools) and [Ubuntu](https://packages.ubuntu.com/stlink-tools) repositories differ from our self-maintained deb-package, we recommend to use the latter instead (see link below). It provides the opportunity to handle and fix user-reported package issues directly within the project and is not redundant to any limitations deriving from external maintenance guidelines.

- Debian Linux: [(Link)](https://github.com/stlink-org/stlink/releases)
- Ubuntu Linux: [(Link)](https://github.com/stlink-org/stlink/releases)
- Arch Linux: [(Link)](https://www.archlinux.org/packages/community/x86_64/stlink)
- Alpine Linux: [(Link)](https://pkgs.alpinelinux.org/packages?name=stlink)
- Fedora: [(Link)](https://src.fedoraproject.org/rpms/stlink)
- Gentoo Linux: [(Link)](https://packages.gentoo.org/packages/dev-embedded/stlink)

**Other Operating Systems**:

- RedHat/CentOS 8: Users can install from [EPEL repository](https://src.fedoraproject.org/rpms/stlink/branch/epel8)
- FreeBSD: Users can install from [freshports](https://www.freshports.org/devel/stlink)

## Installation from source (advanced users)

When there is no executable available for your platform or you need the latest (possible unstable) version you need to compile the toolset yourself. This procedure is explained in the [compiling manual](doc/compiling.md).

## Contributing and versioning

- The semantic versioning scheme is used. Read more at [semver.org](http://semver.org)
- Before creating a pull request, please _ALWAYS_ open a new issue for the discussion of the intended new features. Bugfixes don't require a discussion via a ticket-issue. However they should always be described in a few words as soon as they appear to help others as well.
- Contributors and/or maintainers may submit comments or request changes to patch-proposals and/or pull-requests.
- **ATTENTION: _NEVER EVER_ use the '#' character to count-up single points within a listing as '#' is _exclusively_ reserved for referencing GitHub issues and pull-requests. Otherwise you accidentally introduce false cross references within the project.**
- Please start new forks from the develop branch, as pull requests will go into this branch as well.

Please also refer to our [Contribution Guidelines](CONTRIBUTING.md).

## User Reviews

*I hope it's not to out of topic, but I've been so frustrated with AVR related things on OpenBSD, the fact that stlink built out of the box without needing to touch anything was so relieving. Literally made my whole weekend better!
I take it's thanks to @Crest and also to the stlink-org team (@Nightwalker-87 and @xor-gate it seems) to have made a software that's not unfriendly to the "fringe" OSes.
Thank you <3"* - nbonfils, 11.12.2021