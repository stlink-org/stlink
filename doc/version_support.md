## Supported Operating Systems

### Microsoft Windows

On Windows users should ensure that cmake **3.19** or any later version is installed.<br />
Up on compiling c-make will **automatically** download and install the latest compatible version of `libusb`.

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
- KaOS
- Solus
- Void Linux

Other Linux-/Unix-based Operating Systems:

| Operating System         | libusb                     | cmake      | libgtk-dev  | End of<br />OS-Support |
| ------------------------ | -------------------------- | ---------- | ----------- | ---------------------- |
| CentOS Stream 9 [x64]    | 1.0.26 (`libusbx`)         | 3.**26.5** | 3.24.31     | May 2027               |
| Mageia 9 [x64]           | 1.0.26                     | 3.**26.4** | 3.24.38     |                        |
| Slackware 15 [x64]       | 1.0.**24**                 | 3.**24.1** | 3.24.31     |                        |
| NetBSD 9.x               | 1.0.**24**                 | 3.**21.2** | 3.24.30     |                        |
| FreeBSD 13.x             | 1.0.16-18 (API 0x01000102) | 3.**22.1** | 3.24.31     | Jan 2026               |


## Unsupported Operating Systems (as of Release v1.8.1)

Systems with highlighted versions remain compatible with this toolset until further notice.

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
| Adélie 1.0                               | 1.0.23                     | 3.**23.5** |                        |
| CentOS / Rocky Linux / AlmaLinux 8 [x64] | 1.0.23 (`libusbx`)         | 3.**20.3** | Dec 2021               |
| CentOS 8 Stream [x64]                    | 1.0.23 (`libusbx`)         | 3.**20.2** | May 2024               |

_All other operating systems which are not listed are unsupported._
