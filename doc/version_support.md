
_Source:_ pkgs.org - [libusb](https://pkgs.org/search/?q=libusb); [cmake](https://pkgs.org/search/?q=cmake); [gtk](https://pkgs.org/search/?q=gtk) (as of Apr 2020)


## Supported Operating Systems
### Microsoft Windows

On Windows users should ensure that cmake 3.17.0 is installed.<br />
Up on compiling c-make will check **automatically**, whether `libusb` 1.0.20 or later is present.<br />
If this is not the case, the installation routine will download the latest version (1.0.23 at the time of writing).<br />
Thus no user interaction regarding libusb is necessary.

* Windows 10
* Windows 8.1


### Apple macOS

| Package Repository | libusb<br />version | cmake<br />version | gtk-3<br />version | Supported macOS versions |
| --- | --- | --- | --- | --- |
| homebrew | 1.0.23 | 3.17.0 | 3.24.18<br />gtk+3 | 10.12 (Sierra)- 10.15 (Catalina) |
| MacPorts | 1.0.23 | 3.17.0 | 3.24.18<br />gtk3 | 10.6 (Snow Leopard) - 10.15 (Catalina) |

NOTE: In order to use a STLINK/v1 programmer on macOS, versions 10.13, 10.14 or 10.15 are required.


### Linux-/Unix-based:

| Operating System | libusb<br />version | cmake<br />version | gtk-3<br />version | Notes |
| --- | --- | --- | --- | --- |
| Alpine Edge | 1.0.23 | 3.17.0 | 3.99.0<br />gtk+3.0 | |
| ALT Linux Sisyphus | 1.0.23 | 3.17.0 | 3.24.18<br />libgtk+3 | |
| Arch Linux | 1.0.23 | 3.17.0 | 3.24.18<br />gtk3 | |
| Fedora Rawhide | 1.0.23 | 3.17.0 | 3.24.18<br />gtk3 | | named `libusbx`, but<br />`libusb`-codebase is used |
| KaOS | 1.0.23 | 3.17.0 | 3.24.18<br />gtk3 | |
| OpenMandriva Cooker | 1.0.23 | 3.17.0 | 3.24.18<br />libgtk+3.0<br />lib64gtk+3.0 | |
| PCLinuxOS | 1.0.23<br />lib64usb1.0 | 3.17.0 | 3.24.18<br />lib64gtk+3.0 | |
| Slackware Current | 1.0.23 | 3.17.0 | 3.24.18<br />gtk+3 | |
| Solus | 1.0.23 | 3.16.5 | 3.24.16<br />libgtk-3 | |
| Debian Sid | 1.0.23 | 3.16.3 | 3.24.18<br />libgtk-3 | |
| OpenMandriva Lx 4.1 | 1.0.23 | 3.16.3 | 3.24.13<br />libgtk+3.0<br />lib64gtk+3.0 | |
| Ubuntu 20.04 LTS (Focal Fossa) | 1.0.23 | 3.16.3 | 3.24.17<br />libgtk-3 | |
| openSUSE Tumbleweed | 1.0.23 | 3.16.2 | 3.24.16<br />gtk3 | |
| Alpine 3.11 | 1.0.23 | 3.15.5 | 3.24.13<br />gtk+3.0 | |
| Ubuntu 19.10 (Eoan Ermine) | 1.0.23 | 3.13.4 | 3.24.12<br />libgtk-3 | |
| Mageia Cauldron | 1.0.22 | 3.17.0 | 3.24.18<br />libgtk+3.0<br />lib64gtk+3.0 | |
| NetBSD 9.0 | 1.0.22 | 3.16.1 | 3.24.12<br />gtk+3 | |
| NetBSD 8.1 | 1.0.22 | 3.16.1 | 3.24.12<br />gtk+3 | |
| NetBSD 7.2 | 1.0.22 | 3.16.1 | _N/A_ | |
| Alpine 3.10 | 1.0.22 | 3.14.5 | 3.24.8<br />gtk+3.0 | |
| Fedora 31 | 1.0.22 | 3.14.5 | 3.24.12<br />gtk3 | named `libusbx`, but<br />`libusb`-codebase is used |
| Mageia 7.1 | 1.0.22 | 3.14.3 | 3.24.8<br />libgtk+3.0<br />lib64gtk+3.0 | |
| Fedora 30 | 1.0.22 | 3.14.2 | 3.24.8<br />gtk3 | named `libusbx`, but<br />`libusb`-codebase is used |
| Debian 10 (Buster) | 1.0.22 | 3.13.4 | 3.24.5<br />libgtk-3 | |
| Alpine 3.9 | 1.0.22 | 3.13.0 | 3.24.1<br />gtk+3.0 | |
| CentOS 8 | 1.0.22 | 3.11.4 | 3.22.30<br />gtk3 | named `libusbx`, but<br />`libusb`-codebase is used |
| openSUSE Leap 15.2 | 1.0.21 | 3.15.5 | 3.24.14<br />gtk3 | |
| openSUSE Leap 15.1 | 1.0.21 | 3.10.2 | 3.22.30<br />gtk3 | |
| Ubuntu 18.04 LTS (Bionic Beaver) | 1.0.21 | 3.10.2 | 3.22.30<br />libgtk-3 | |
| Debian 9 (Stretch) | 1.0.21 | 3.7.2 | 3.22.11<br />libgtk-3 | |
| Slackware 14.2 | **1.0.20** | 3.5.2 | 3.18.9<br />gtk+3 | |
| Ubuntu 16.04 LTS (Xenial Xerus) | **1.0.20** | 3.5.1 | 3.18.9<br />libgtk-3 | |
| OpenMandriva Lx 3.0 | **1.0.20** | **3.4.2** | 3.18.9<br />libgtk+3.0<br />lib64gtk+3.0 | |
| FreeBSD 13 | **1.0.16** - 1.0.18 | 3.15.5 | 3.24.10<br />gtk3 | linux_libusb-13.0r358841<br />(integrated) |
| FreeBSD 12 | **1.0.16** - 1.0.18 | 3.15.5 | 3.24.10<br />gtk3 | linux_libusb-11.0r261448_4<br />(integrated) |
| FreeBSD 11 | **1.0.16** - 1.0.18 | 3.15.5 | 3.24.10<br />gtk3 | linux_libusb-11.0r261448_4<br />(integrated) |


## Unsupported Operating Systems (as of Release v1.6.1)

| Operating System | libusb<br />version | cmake<br />version | End of<br />OS-Support | Notes |
| --- | --- | --- | --- | --- |
| CentOS 7 | 1.0.21 | **2.8.12.2** | | named `libusbx`, but<br />`libusb`-codebase is used |
| Debian 8 (Jessie) | 1.0.**19** | 3.**0.2** | Jun 2020 |
| Ubuntu 14.04 LTS (Trusty Tahr) | 1.0.**17** | **2.8.12.2** | Apr 2019 |
| CentOS 6 | 1.0.**9** | **2.8.12.2** | Dec 2020 | named `libusbx`, but<br />`libusb`-codebase is used |
| Slackware 14.1 | 1.0.**9** | **2.8.12** | |
| Slackware 14.0 | 1.0.**9** | **2.8.8** | |

_All other operating systems which are not listed are unsupported._

Author: nightwalker-87
