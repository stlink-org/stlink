## Supported Operating Systems

### Microsoft Windows

On Windows users should ensure that cmake **3.16.3** or any later version is installed.<br />
Up on compiling c-make will **automatically** download and install the latest compatible version of `libusb`.

- Windows 10
- Windows 11

### Linux-/Unix-based:

Actively maintained versions of:
- Debian
- Ubuntu
- Fedora
- openSUSE
- OpenMandriva
- Arch Linux
- FreeBSD [libusb 1.0.16-18 (API 0x01000102)]
- NetBSD
- OpenBSD

Other Linux-/Unix-based Operating Systems:

| Operating System         | libusb                     | cmake      | libgtk-dev  | End of<br />OS-Support |
| ------------------------ | -------------------------- | ---------- | ----------- | ---------------------- |
| KaOS [x64]               | 1.0.27                     | 3.30.2     | 3.24.43     |                        |
| Void Linux [x64]         | 1.0.27                     | 3.30.1     | 3.24.42     |                        |
| CentOS 9 Stream [x64]    | 1.0.26 (`libusbx`)         | 3.26.5     | 3.24.31     |                        |
| Mageia 9 [x64]           | 1.0.26                     | 3.26.4     | 3.24.38     |                        |
| Solus [x64]              | 1.0.26                     | 3.24.1     | 3.24.38     |                        |
| ALT Linux P10 [x64]      | 1.0.26                     | 3.23.2     | 3.24.32     |                        |
| PCLinuxOS [x64]          | (?)                        | 3.30.2     | 3.24.38     |                        |
|                          |                            |            |             |                        |
| Slackware 15 [x64]       | 1.0.**24**                 | 3.24.1     | 3.24.31     |                        |
| NetBSD 9.x               | 1.0.**24**                 | 3.**21.2** | 3.24.**30** |                        |
| Adélie 1.0               | 1.0.**23**                 | 3.23.5     | 3.24.**30** |                        |
| Ubuntu 20.04 LTS (Focal) | 1.0.**23**                 | 3.**16.3** | 3.24.**18** | May 2025               |
| FreeBSD 13.x             | 1.0.16-18 (API 0x01000102) | 3.**22.1** | 3.24.31     | Jan 2026               |


## Unsupported Operating Systems (as of Release v1.8.1)

Systems with highlighted versions remain compatible with this toolset.

| Operating System                         | libusb                     | cmake      | End of<br />OS-Support |
| ---------------------------------------- | -------------------------- | ---------- | ---------------------- |
| FreeBSD 12.x                             | 1.0.16-18 (API 0x01000102) | 3.**22.1** | Dec 2023               |
| Alpine 3.15                              | 1.0.**24**                 | 3.**21.3** | Nov 2023               |
| Fedora 35 [x64]                          | 1.0.**24**                 | 3.**21.3** | Dec 2022               |
| Alpine 3.14                              | 1.0.**24**                 | 3.**20.3** | May 2023               |
| Fedora 34 [x64]                          | 1.0.**24** (`libusbx`)     | 3.**19.7** | Jun 2022               |
| NetBSD 8.x                               | 1.0.**24**                 | 3.**19.7** | May 2024               |
| OpenMandriva Lx 4.2                      | 1.0.**24**                 | 3.**19.3** | Mar 2023               |
| Mageia 8                                 | 1.0.**24**                 | 3.**19.2** | Aug 2022               |
| Debian 11 (Bullseye)                     | 1.0.**24**                 | 3.**18.4** | Jun 2024               |
| Alpine 3.13                              | 1.0.**24**                 | 3.**18.4** | Nov 2022               |
| Ubuntu 21.04 (Hirsute)                   | 1.0.**24**                 | 3.**18.4** | Jan 2022               |
| CentOS / Rocky Linux / AlmaLinux 8 [x64] | 1.0.**23** (`libusbx`)     | 3.**20.3** | Dec 2021               |
| CentOS 8 Stream [x64]                    | 1.0.**23** (`libusbx`)     | 3.**20.2** | May 2024               |
| Fedora 33 [x64]                          | 1.0.**23** (`libusbx`)     | 3.**18.3** | Nov 2021               |
| Alpine 3.12                              | 1.0.**23**                 | 3.**17.2** | May 2022               |
| openSUSE Leap 15.3 [x64]                 | 1.0.21                     | 3.**17.0** | Dec 2022               |
| Fedora 32 [x64]                          | 1.0.**23** (`libusbx`)     | 3.**17.0** | May 2021               |
| openSUSE Leap 15.2 [x64]                 | 1.0.21                     | 3.**17.0** | Dec 2021               |
| Ubuntu 20.10 (Groovy)                    | 1.0.**23**                 | 3.**16.3** | Jul 2021               |
| NetBSD 7.x                               | 1.0.22                     | 3.16.1     | Jun 2020               |
| Alpine 3.11                              | 1.0.**23**                 | 3.15.5     | Nov 2021               |
| FreeBSD 11.x                             | 1.0.16-18 (API 0x01000102) | 3.15.5     | Sep 2021               |
| ALT Linux P9                             | 1.0.22                     | 3.**16.3** |                        |
| Alpine 3.10                              | 1.0.22                     | 3.14.5     | May 2021               |
| Fedora 31 [x64]                          | 1.0.22 (`libusbx`)         | 3.14.5     | Nov 2020               |
| Mageia 7.1                               | 1.0.22                     | 3.14.3     | Jun 2021               |
| Fedora 30                                | 1.0.22 (`libusbx`)         | 3.14.2     | May 2020               |
| Ubuntu 19.10 (Eoan)                      | 1.0.**23**                 | 3.13.4     | Jul 2020               |
| Alpine 3.9                               | 1.0.22                     | 3.13.0     | Jan 2021               |
| Debian 10 (Buster)                       | 1.0.22                     | 3.13.4     | Jun 2024               |
| Ubuntu 18.04 LTS (Bionic)                | 1.0.21                     | 3.10.2     | Apr 2023               |
| openSUSE Leap 15.1 [x64]                 | 1.0.21                     | 3.10.2     | Jan 2021               |
| Debian 9 (Stretch)                       | 1.0.21                     | 3.7.2      | Jun 2022               |
| Slackware 14.2                           | 1.0.20                     | 3.5.2      | Jan 2024               |
| OpenMandriva Lx 3.0x                     | 1.0.20                     | 3.4.2      | Jul 2019               |
| CentOS / Rocky Linux / AlmaLinux 7 [x64] | 1.0.21 (`libusbx`)         | 2.8.12.2   | Jun 2024               |

_All other operating systems which are not listed are unsupported._
