
Sources: [pkgs.org - libusb](https://pkgs.org/search/?q=libusb) & [pkgs.org - cmake](https://pkgs.org/search/?q=cmake) (as of Mar 2020):


## Supported Operating Systems
### Microsoft Windows

On Windows users should ensure that cmake 3.17.0 is installed.<br />
Up on compiling c-make will check **automatically**, whether `libusb` 1.0.20 or later is present.<br />
If this is not the case, the installation routine will download the latest version (1.0.23 at the time of writing).<br />
Thus no user interaction regarding libusb is necessary.

* Windows 10
* Windows 8.1


### Apple macOS

| Package Repository | libusb<br />version | cmake<br />version | Supported macOS versions |
| --- | --- | --- | --- |
| homebrew | 1.0.23 | 3.17.0 | 10.12 (Sierra)- 10.15 (Catalina) |
| MacPorts | 1.0.23 | 3.17.0 | 10.6 (Snow Leopard) - 10.15 (Catalina) |


### Linux-/Unix-based:

| Operating System | libusb<br />version | cmake<br />version | Notes |
| --- | --- | --- | --- |
| Alpine Edge | 1.0.23 | 3.17.0 | |
| ALT Linux Sisyphus | 1.0.23 | 3.17.0 | |
| Arch Linux | 1.0.23 | 3.17.0 | |
| Fedora Rawhide | 1.0.23 | 3.17.0 | named `libusbx`, but `libusb`-codebase is used |
| KaOS | 1.0.23 | 3.17.0 | |
| OpenMandriva Cooker | 1.0.23 | 3.17.0 | |
| PCLinuxOS | 1.0.23 | 3.17.0 | named `lib64usb1.0_0-1.0.23-1pclos2019.x86_64` |
| Slackware Current | 1.0.23 | 3.17.0 | |
| Solus | 1.0.23 | 3.16.5 | |
| Debian Sid | 1.0.23 | 3.16.3 | |
| OpenMandriva Lx 4.1 | 1.0.23 | 3.16.3 | |
| Ubuntu 20.04 LTS (Focal Fossa) | 1.0.23 | 3.16.3 | |
| openSUSE Tumbleweed | 1.0.23 | 3.16.2 | |
| Alpine 3.11 | 1.0.23 | 3.15.5 | |
| Ubuntu 19.10 (Eoan Ermine) | 1.0.23 | 3.13.4 | |
| Mageia Cauldron | 1.0.22 | 3.17.0 | |
| NetBSD 9.0 | 1.0.22 | 3.16.1 | |
| NetBSD 8.1 | 1.0.22 | 3.16.1 | |
| NetBSD 7.2 | 1.0.22 | 3.16.1 | |
| Alpine 3.10 | 1.0.22 | 3.14.5 | |
| Fedora 31 | 1.0.22 | 3.14.5 | named `libusbx`, but `libusb`-codebase is used |
| Mageia 7.1 | 1.0.22 | 3.14.3 | |
| Fedora 30 | 1.0.22 | 3.14.2 | named `libusbx`, but `libusb`-codebase is used |
| Debian 10 (Buster) | 1.0.22 | 3.13.4 | |
| Alpine 3.9 | 1.0.22 | 3.13.0 | |
| CentOS 8 | 1.0.22 | 3.11.4 | named `libusbx`, but `libusb`-codebase is used |
| openSUSE Leap 15.2 | 1.0.21 | 3.15.5 | |
| openSUSE Leap 15.1 | 1.0.21 | 3.10.2 | |
| Ubuntu 18.04 LTS (Bionic Beaver) | 1.0.21 | 3.10.2 | |
| Debian 9 (Stretch) | 1.0.21 | 3.7.2 | |
| Slackware 14.2 | **1.0.20** | 3.5.2 | |
| Ubuntu 16.04 LTS (Xenial Xerus) | **1.0.20** | 3.5.1 | |
| OpenMandriva Lx 3.0 | **1.0.20** | **3.4.2** | |
| FreeBSD 13 | **1.0.16** - 1.0.18 | 3.15.5 | linux_libusb-13.0r358841 (integrated) |
| FreeBSD 12 | **1.0.16** - 1.0.18 | 3.15.5 | linux_libusb-11.0r261448_4 (integrated) |
| FreeBSD 11 | **1.0.16** - 1.0.18 | 3.15.5 | linux_libusb-11.0r261448_4 (integrated) |


## Unsupported Operating Systems as of Release 1.6.1 (2020)

| Operating System | libusb<br />version | cmake<br />version | End of OS-Support | Notes |
| --- | --- | --- | --- | --- |
| CentOS 7 | 1.0.21 | **2.8.12.2** | | named `libusbx`, but `libusb`-codebase is used |
| Debian 8 (Jessie) | 1.0.**19** | 3.0.2 | Jun 2020 |
| Ubuntu 14.04 LTS (Trusty Tahr) | 1.0.**17** | **2.8.12.2** | Apr 2019 |
| CentOS 6 | 1.0.**9** | **2.8.12.2** | Dec 2020 | named `libusbx`, but `libusb`-codebase is used |
| Slackware 14.1 | 1.0.**9** | **2.8.12** | |
| Slackware 14.0 | 1.0.**9** | **2.8.8** | |

_All other operating systems which are not listed are unsupported._

Author: nightwalker-87
