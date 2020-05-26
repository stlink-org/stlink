###
# Installation instructions for STLINK/v1 driver
###

When connecting to the STLINK/v1 on macOS via USB, the system claims the programmer as a SCSI device. Thus libusb is not able to initialise and establish a connection to it. To solve this issue Marco Cassinerio (marco.cassinerio@gmail.com) has created a so called "codeless driver" which claims the device. It is of higher priority then the default apple mass storage driver, what allows the device to be accessed through libusb.

To make use of this alternative approach one needs to go through the following steps:

1) Install the macOS Kernel Extension (kext):
 - Open a terminal console and navigate to this subdirectory `/stlinkv1_macos_driver`
 - Use the command ```sudo sh ./install.sh``` to install the appropiate kext for your system version.

2) Install the ST-Link-v1 driver from this subdirectory `/stlinkv1_macos_driver` by executing: ```sudo make osx_stlink_shield```. This should result in the following output:

```
Requesting load of /System/Library/Extensions/stlink_shield.kext.
/System/Library/Extensions/stlink_shield.kext loaded successfully (or already loaded).
```

3) Configure System Integrity Protection (SIP)

The above system security setting introduced by Apple with OS X El Capitan (10.11) in 2015 is active per default
and prevents the operating system amongst other things to load unsigned Kernel Extension Modules (kext).
Thus the STLINK/v1 driver supplied with the tools, which installs as a kext, remains not functional,
until SIP is fully deactivated.

Without SIP-deactivation, st-util fails to detect a STLINK/v1 device:

```
st-util -1
st-util $VERSION-STRING$
WARN src/sg.c: Failed to find an stlink v1 by VID:PID
ERROR src/sg.c: Could not open stlink device
```

In order to deactivate SIP, boot into the recovery mode and run ```csrutil disable``` in a terminal console window.

4) Reboot the system.

5) Verify correct detection of the STLINK/v1 device with the following input: `st-util -1`
You should then see a similar output like in this example:

```
INFO common.c: Loading device parameters....
INFO common.c: Device connected is: F1 High-density device, id 0x10036414
INFO common.c: SRAM size: 0x10000 bytes (64 KiB), Flash: 0x80000 bytes (512 KiB) in pages of 2048 bytes
INFO sg.c: Successfully opened a stlink v1 debugger
INFO gdb-server.c: Chip ID is 00000414, Core ID is  1ba01477.
INFO gdb-server.c: Listening at *:4242...
```
