_Source:_ [pkgs.org](https://pkgs.org) - libusb, cmake, gtk, libgtk (as of Apr 2023)

## Supported Operating Systems

### Microsoft Windows

On Windows users should ensure that cmake **3.13.0** or any later version is installed.<br />
Up on compiling c-make will **automatically** download and install the latest compatible version of `libusb`.

- Windows 10
- Windows 11

### Linux-/Unix-based:

Maintained versions of:
- Debian
- Ubuntu
- Fedora
- openSUSE
- OpenMandriva
- Arch Linux
- FreeBSD
- NetBSD
- OpenBSD

Other Linux-/Unix-based Operating Systems:

| Operating System         | libusb                         | cmake      | libgtk-dev  | End of<br />OS-Support |
| ------------------------ | ------------------------------ | ---------- | ----------- | ---------------------- |
| Debian Sid               | 1.0.24                         | 3.22.1     | 3.24.31     |                        |
| Debian 11 (Bullseye)     | 1.0.24                         | 3.**18.4** | 3.24.24     |                        |
| Debian 10 (Buster)       | 1.0.**22**                     | 3.**13.4** | 3.24.**5**  | Jun 2024               |
|                          |                                |            |             |                        |
| Ubuntu 20.04 LTS (Focal) | 1.0.23                         | 3.**16.3** | 3.24.**18** | May 2025               |
|                          |                                |            |             |                        |
| FreeBSD 13.x             | 1.0.**16-18** (API 0x01000102) | 3.22.1     | 3.24.31     |                        |
|                          |                                |            |             |                        |
| NetBSD 9.x               | 1.0.24                         | 3.21.2     | 3.24.30     |                        |
| NetBSD 8.x               | 1.0.24                         | 3.**19.7** | 3.24.27     |                        |
|                          |                                |            |             |                        |
| CentOS 9 Stream [x64]    | 1.0.24 (`libusbx`)             | 3.20.3     | 3.24.30     |                        |
| CentOS 8 Stream [x64]    | 1.0.23 (`libusbx`)             | 3.20.2     | 3.**22.30** | May 2024               |
|                          |                                |            |             |                        |
| ALT Linux Sisyphus       | 1.0.24                         | 3.22.1     | 3.24.31     |                        |
| ALT Linux P10            | 1.0.24                         | 3.20.5     | 3.24.31     |                        |
| ALT Linux P9             | 1.0.**22**                     | 3.**16.3** | 3.24.29     |                        |
|                          |                                |            |             |                        |
| KaOS [x64]               | 1.0.24                         | 3.22.1     | 3.24.31     |                        |
| Mageia Cauldron          | 1.0.24                         | 3.22.1     | 3.24.31     |                        |
| PCLinuxOS [x64]          | (?)                            | 3.22.1     | 3.24.31     |                        |
| Solus [x64]              | 1.0.24                         | 3.22.1     | 3.24.30     |                        |
| Void Linux               | 1.0.24                         | 3.22.1     | 3.24.31     |                        |
| Slackware Current        | 1.0.24                         | 3.21.4     | 3.24.31     |                        |
| Adélie 1.0               | 1.0.23                         | 3.**16.4** | 3.24.23     |                        |

## Unsupported Operating Systems (as of Release v1.8.0)

Systems with highlighted versions remain compatible with this toolset.

| Operating System                         | libusb                         | cmake      | End of<br />OS-Support |
| ---------------------------------------- | ------------------------------ | ---------- | ---------------------- |
| FreeBSD 12.x                             | 1.0.**16-18** (API 0x01000102) | 3.**22.1** | Dec 2023               |
| Alpine 3.15                              | 1.0.**24**                     | 3.**21.3** | Nov 2023               |
| Fedora 35 [x64]                          | 1.0.**24**                     | 3.**21.3** | Dec 2022               |
| Alpine 3.14                              | 1.0.**24**                     | 3.**20.3** | May 2023               |
| CentOS / Rocky Linux / AlmaLinux 8 [x64] | 1.0.**23** (`libusbx`)         | 3.**20.3** | Dec 2021               |
| Fedora 34 [x64]                          | 1.0.**24** (`libusbx`)         | 3.**19.7** | Jun 2022               |
| OpenMandriva Lx 4.2                      | 1.0.**24**                     | 3.**19.3** | Mar 2023               |
| Mageia 8                                 | 1.0.**24**                     | 3.**19.2** | Aug 2022               |
| Alpine 3.13                              | 1.0.**24**                     | 3.**18.4** | Nov 2022               |
| Ubuntu 21.04 (Hirsute)                   | 1.0.**24**                     | 3.**18.4** | Jan 2022               |
| Fedora 33 [x64]                          | 1.0.**23** (`libusbx`)         | 3.**18.3** | Nov 2021               |
| Alpine 3.12                              | 1.0.**23**                     | 3.**17.2** | May 2022               |
| openSUSE Leap 15.3 [x64]                 | 1.0.21                         | 3.**17.0** | Dec 2022               |
| Fedora 32 [x64]                          | 1.0.**23** (`libusbx`)         | 3.**17.0** | May 2021               |
| openSUSE Leap 15.2 [x64]                 | 1.0.21                         | 3.**17.0** | Dec 2021               |
| Ubuntu 20.10 (Groovy)                    | 1.0.**23**                     | 3.**16.3** | Jul 2021               |
| NetBSD 7.x                               | 1.0.**22**                     | 3.**16.1** | Jun 2020               |
| Alpine 3.11                              | 1.0.**23**                     | 3.**15.5** | Nov 2021               |
| FreeBSD 11.x                             | 1.0.**16-18** (API 0x01000102) | 3.**15.5** | Sep 2021               |
| Alpine 3.10                              | 1.0.**22**                     | 3.**14.5** | May 2021               |
| Fedora 31 [x64]                          | 1.0.**22**(`libusbx`)          | 3.**14.5** | Nov 2020               |
| Mageia 7.1                               | 1.0.**22**                     | 3.**14.3** | Jun 2021               |
| Fedora 30                                | 1.0.**22**(`libusbx`)          | 3.**14.2** | May 2020               |
| Ubuntu 19.10 (Eoan)                      | 1.0.**23**                     | 3.**13.4** | Jul 2020               |
| Alpine 3.9                               | 1.0.**22**                     | 3.**13.0** | Jan 2021               |
| Ubuntu 18.04 LTS (Bionic)                | 1.0.21                         | 3.10.2     | Apr 2023               |
| openSUSE Leap 15.1 [x64]                 | 1.0.21                         | 3.10.2     | Jan 2021               |
| Debian 9 (Stretch)                       | 1.0.21                         | 3.7.2      | Jun 2022               |
| Slackware 14.2                           | 1.0.20                         | 3.5.2      |                        |
| OpenMandriva Lx 3.0x                     | 1.0.20                         | 3.4.2      |                        |
| CentOS / Rocky Linux / AlmaLinux 7 [x64] | 1.0.21 (`libusbx`)             | 2.8.12.2   | Jun 2024               |

_All other operating systems which are not listed are unsupported._

Author: nightwalker-87
