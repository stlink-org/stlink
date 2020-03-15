Welcome to the stlink wiki! (Archived: 2014-08-11)

Misc:
Windows pre-compiled binaries are available at http://www.emb4fun.de/archive/stlink/index.html

FAQ:
Q: Where can I get help? Is there a forum or maybe a mailing list?

A: todo

Q: How can I install stlink and flash binaries on Mac OS X ?

A: **20140811: ** installed on Mac OS X 10.9.4 with ports method, however STlink v2 does not seem to work with libusb if you upgrade to the newest firmware !! Only older firmware on STlink v2 works.

[https://coderwall.com/p/oznj_q](https://coderwall.com/p/oznj_q)

`sudo port install libusb automake autoconf pkgconfig`

`aclocal --force -I /opt/local/share/aclocal`

`git clone https://github.com/texane/stlink.git stlink-utility`

`cd stlink-utility`

`./autogen.sh`

`./configure`

`make`


Then trying to flash the image with STLINK v2 :

`./st-flash write ~/Downloads/irq.bin 0x8000000`

> libusb_handle_events() timeout
>
> [!] send_recv

After downgrading the firmware, flashing works
ST-Link/V2 debugger with downgraded V2.14.3 firmware:

https://drive.google.com/folderview?id=0Bzv7UpKpOQhnbXJVVEg4VUo2M1k

as described here:

http://community.spark.io/t/how-to-flash-a-brand-new-freshly-soldered-stm32f103-chip/3906

`./st-flash write ~/Downloads/irq.bin 0x8000000`

> 2014-08-11T23:14:52 INFO src/stlink-common.c: Loading device parameters....
>
> 2014-08-11T23:14:52 INFO src/stlink-common.c: Device connected is: F1 Medium-density device, id 0x20036410
>
> 2014-08-11T23:14:52 INFO src/stlink-common.c: SRAM size: 0x5000 bytes (20 KiB), Flash: 0x20000 bytes (128 KiB) in
> pages of 1024 bytes
>
> 2014-08-11T23:14:52 INFO src/stlink-common.c: Attempting to write 24904 (0x6148) bytes to stm32 address: 134217728
> (0x8000000)
>
> Flash page at addr: 0x08006000 erased
>
> 2014-08-11T23:14:53 INFO src/stlink-common.c: Finished erasing 25 pages of 1024 (0x400) bytes
>
> 2014-08-11T23:14:53 INFO src/stlink-common.c: Starting Flash write for VL/F0 core id
>
> 2014-08-11T23:14:53 INFO src/stlink-common.c: Successfully loaded flash loader in sram 24/24 pages written
>
> 2014-08-11T23:14:54 INFO src/stlink-common.c: Starting verification of write complete
>
> 2014-08-11T23:14:54 INFO src/stlink-common.c: Flash written and verified! jolly good!


Installation of openOCD on Mac OS X with STlink-v2 has been successful:

`sudo port install openocd +jlink +stlink +ft2232`

`/opt/local/bin/openocd --version`

> Open On-Chip Debugger 0.7.0 (2014-08-11-22:12)

`openocd -f /opt/local/share/openocd/scripts/interface/stlink-v2.cfg -f /opt/local/share/openocd/scripts/target/stm32f1x_stlink.cfg `

> Open On-Chip Debugger 0.8.0 (2014-08-11-15:36)
>
> Licensed under GNU GPL v2
>
> For bug reports, read http://openocd.sourceforge.net/doc/doxygen/bugs.html
>
> Info : This adapter doesn't support configurable speed
>
> Info : STLINK v2 JTAG v21 API v2 SWIM v4 VID 0x0483 PID 0x3748
>
> Info : using stlink api v2
>
> Info : Target voltage: 3.244269
>
> Info : stm32f1x.cpu: hardware has 6 breakpoints, 4 watchpoints

and connecting to the server through telnet yields a successful installation

`telnet localhost 4444`

> Connected to localhost.
>
> Escape character is '^]'.
>
> Open On-Chip Debugger
