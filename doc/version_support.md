_Source:_ pkgs.org - [libusb](https://pkgs.org/search/?q=libusb); [cmake](https://pkgs.org/search/?q=cmake); [gtk](https://pkgs.org/search/?q=gtk) (as of May 2021)

## Supported Operating Systems

### Microsoft Windows

On Windows users should ensure that cmake 3.20.2 or any later version is installed.<br />
Up on compiling c-make will **automatically** download and install the latest compatible version of `libusb` (1.0.23 at the time of writing).

- Windows 10
- Windows 8.1

### Apple macOS

| Package Repository | libusb<br />version | cmake<br />version | gtk-3<br />version | Supported macOS versions |
| ------------------ | ------------------- | ------------------ | ------------------ | ------------------------ |
| homebrew           | 1.0.24              | 3.20.2             | 3.24.29<br />gtk+3 | 10.9 - 11.x              |
| MacPorts           | 1.0.24              | 3.20.2             | 3.24.29<br />gtk3  | 10.4 - 11.x              |

NOTE: In order to use a STLINK/V1 programmer on macOS, versions 10.14 or 10.15 are required.

### Linux-/Unix-based:

| Operating System          | libusb                           | cmake     | gtk-3       | Notes                    |
| ------------------------- | -------------------------------- | --------- | ----------- | ------------------------ |
| Debian Sid                | 1.0.24                           | 3.18.4    | 3.24.24     |                          |
| Debian 11 (Bullseye)      | 1.0.24                           | 3.18.4    | 3.24.24     |                          |
| Debian 10 (Buster)        | 1.0.**22**                       | 3.13.4    | 3.24.**5**  |                          |
| Debian 9 (Stretch)        | 1.0.**21**                       | **3.7.2** | **3.22.11** | End of Support: Jun 2022 |
|                           |                                  |           |             |                          |
| Ubuntu 21.04 (Hirsute)    | 1.0.24                           | 3.18.4    | 3.24.25     | End of Support: Jan 2022 |
| Ubuntu 20.04 LTS (Focal)  | 1.0.23                           | 3.16.3    | 3.24.**18** |                          |
| Ubuntu 18.04 LTS (Bionic) | 1.0.**21**                       | 3.10.2    | **3.22.30** | End of Support: Apr 2023 |
|                           |                                  |           |             |                          |
| Fedora Rawhide [x64]      | 1.0.24 (`libusbx`)               | 3.20.2    | 3.24.29     |                          |
| Fedora 34 [x64]           | 1.0.24 (`libusbx`)               | 3.19.7    | 3.24.28     |                          |
| Fedora 33 [x64]           | 1.0.23 (`libusbx`)               | 3.18.3    | 3.24.23     |                          |
|                           |                                  |           |             |                          |
| openSUSE Tumbleweed [x64] | 1.0.24                           | 3.20.1    | 3.24.29     |                          |
| openSUSE Leap 15.3 [x64]  | 1.0.**21**                       | 3.17.0    | 3.24.20     |                          |
| openSUSE Leap 15.2 [x64]  | 1.0.**21**                       | 3.17.0    | 3.24.**14** | End of Support: Dec 2021 |
|                           |                                  |           |             |                          |
| Alpine 3.14               | 1.0.24                           | 3.20.3    | 4.2.1       |                          |
| Alpine 3.13               | 1.0.24                           | 3.18.4    | 3.24.23     | End of Support: Nov 2022 |
| Alpine 3.12               | 1.0.23                           | 3.17.2    | 3.24.22     | End of Support: May 2022 |
| Alpine 3.11               | 1.0.23                           | 3.15.5    | 3.24.**13** | End of Support: Nov 2021 |
|                           |                                  |           |             |                          |
| FreeBSD 13.x              | 1.0.**16 - 18** (API 0x01000102) | 3.20.2    | 3.24.27     |                          |
| FreeBSD 12.x              | 1.0.**16 - 18** (API 0x01000102) | 3.19.6    | 3.24.27     |                          |
| FreeBSD 11.x              | 1.0.**16 - 18** (API 0x01000102) | 3.15.5    | 3.24.27     | End of Support: Sep 2021 |
|                           |                                  |           |             |                          |
| Arch Linux                | 1.0.24                           | 3.20.2    | 3.24.29     |                          |
| KaOS [x64]                | 1.0.24                           | 3.20.2    | 3.24.29     |                          |
| Mageia Cauldron           | 1.0.24                           | 3.20.2    | 3.24.29     |                          |
| OpenMandriva Cooker       | 1.0.24                           | 3.20.2    | 3.24.29     |                          |
| PCLinuxOS [x64]           | 1.0.24                           | 3.20.2    | 3.24.29     |                          |
| Slackware Current         | 1.0.24                           | 3.20.2    | 3.24.28     |                          |
| Solus [x64]               | 1.0.24                           | 3.20.2    | 3.24.29     |                          |
| ALT Linux Sisyphus        | 1.0.24                           | 3.19.7    | 3.24.29     |                          |
| NetBSD 9.x                | 1.0.24                           | 3.19.7    | 3.24.27     |                          |
| NetBSD 8.x                | 1.0.24                           | 3.19.7    | 3.24.27     |                          |
| OpenMandriva Lx 4.2       | 1.0.24                           | 3.19.3    | 3.24.24     |                          |
| Mageia 8                  | 1.0.24                           | 3.19.2    | 3.24.24     | End of Support: Aug 2022 |
| CentOS 8 Stream [x64]     | 1.0.23 (`libusbx`)               | 3.18.2    | **3.22.30** |                          |
| Adélie 1.0                | 1.0.23                           | 3.16.4    | 3.24.23     |                          |
| ALT Linux P9              | 1.0.**22**                       | 3.16.3    | 3.24.**11** |                          |
| AlmaLinux 8               | 1.0.23 (`libusbx`)               | 3.11.4    | 3.24.32     |                          |
| CentOS 8 [x64]            | 1.0.23 (`libusbx`)               | 3.11.4    | **3.22.30** | End of Support: Dec 2021 |

## Unsupported Operating Systems (as of Release v1.7.1)

Systems with highlighted versions remain compatible with this toolset.

| Operating System          | libusb                 | cmake      | End of<br />OS-Support |
| ------------------------- | ---------------------- | ---------- | ---------------------- |
| Fedora 32 [x64]           | **1.0.23** (`libusbx`) | **3.17.0** | May 2021               |
| Ubuntu 20.10 (Groovy)     | **1.0.23**             | **3.16.3** | Jul 2021               |
| NetBSD 7.x                | **1.0.22**             | **3.16.1** | Jun 2020               |
| Alpine 3.10               | **1.0.22**             | **3.14.5** | May 2021               |
| Fedora 31 [x64]           | **1.0.22** (`libusbx`) | **3.14.5** | Nov 2020               |
| Mageia 7.1                | **1.0.22**             | **3.14.3** | Jun 2021               |
| Fedora 30                 | **1.0.22** (`libusbx`) | **3.14.2** | May 2020               |
| Ubuntu 19.10 (Eoan)       | **1.0.23**             | **3.13.4** | Jul 2020               |
| Alpine 3.9                | **1.0.22**             | **3.13.0** | Jan 2021               |
| openSUSE Leap 15.1 [x64]  | **1.0.21**             | **3.10.2** | Jan 2021               |
| Slackware 14.2            | 1.0.20                 | 3.5.2      |                        |
| Ubuntu 16.04 LTS (Xenial) | 1.0.20                 | 3.5.1      | Apr 2021               |
| OpenMandriva Lx 3.0x      | 1.0.20                 | 3.4.2      |                        |
| Debian 8 (Jessie)         | 1.0.19                 | 3.0.2      | Jun 2020               |
| CentOS 7 [x64]            | 1.0.21 (`libusbx`)     | 2.8.12.2   | Jun 2024               |
| Ubuntu 14.04 LTS (Trusty) | 1.0.17                 | 2.8.12.2   | Apr 2019               |
| CentOS 6                  | 1.0.9  (`libusbx`)     | 2.8.12.2   | Nov 2020               |
| Slackware 14.1            | 1.0.9                  | 2.8.12     |                        |
| Slackware 14.0            | 1.0.9                  | 2.8.8      |                        |

_All other operating systems which are not listed are unsupported._

Author: nightwalker-87
