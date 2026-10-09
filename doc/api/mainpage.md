# stlink-lib API {#mainpage}

stlink-lib is the library behind the stlink tools (st-flash, st-info, st-util,
st-trace, st-server and stlink-gui). It talks to ST-LINK/V1, V2, V2-1 and V3
debug probes via libusb (or via st-server over TCP) and programs and debugs
STM32 targets through them.

> **Status:** the library interface is being prepared for use by other applications.
> It is not stable yet: functions, types and headers may change between
> releases.

## Modules

| Module | Content |
|---|---|
| @ref api_device | open, probe and close devices (USB, remote) |
| @ref api_adapter | version, mode, SWD clock and target voltage of the ST-LINK |
| @ref api_target | connect, identify, halt, run, step and reset the target |
| @ref api_memory | memory and core register access |
| @ref api_flash | erase, program and verify flash and OTP memory |
| @ref api_sram_file | files, Intel HEX, SRAM download |
| @ref api_option_bytes | option bytes and option control registers |
| @ref api_chipid | chip database from the `*.chip` files |
| @ref api_logging | log handler and log level |
| @ref api_progress | progress handler |
| @ref api_util | helpers |
| @ref api_backend | the backend interface (stlink_backend_t) |
| @ref api_legacy | deprecated functions |

## Example

Program a binary file into the flash of the first ST-LINK found:

```c
#include <stlink.h>
#include <chipid.h>
#include <common_flash.h>
#include <logging.h>
#include <usb.h>

int main(void) {
    stlink_log_set_level(STLINK_LOG_WARN);  // log level of the application (default: debug)
    init_chipids(NULL);                     // load the chip database before opening a device

    stlink_t *sl = stlink_open_usb(UWARN, CONNECT_NORMAL, NULL, 0);
    if(sl == NULL) { return 1; }            // no ST-LINK found

    int32_t err = -1;
    if(sl->flash_size != 0 &&               // target identified
       stlink_force_debug(sl) == 0) {       // halt the core
        err = stlink_fwrite_flash(sl, "firmware.bin", sl->flash_base, SECTION_ERASE);
        stlink_reset(sl, RESET_AUTO);
    }

    stlink_exit_debug_mode(sl);             // detach cleanly
    stlink_close(sl);
    return err ? 1 : 0;
}
```

Build against the installed library, e.g.
`cc example.c -I/usr/local/include/stlink $(pkg-config --cflags --libs libusb-1.0) -lstlink`
(the headers are installed into the `stlink` subdirectory of the include directory, except on
Windows; `usb.h` needs the libusb headers).

## Conventions

- **Return values:** functions returning `int32_t` return 0 on success and
  -1 (or another non-zero value) on failure, unless documented otherwise.
  The reason of a failure is logged (see @ref api_logging).
- **Handles:** a stlink_t handle comes from stlink_open_usb(),
  stlink_open_remote(), stlink_open_remote_str() or stlink_probe_usb() and is
  released with stlink_close(). Applications read the target description
  (flash_base, flash_size, flash_pgsz, sram_base, sram_size, chip_id, ...)
  directly from the handle. They may set `verbose` and `opt`, but must not
  change other fields.
- **Data buffer:** stlink_read_mem32(), stlink_write_mem32() and
  stlink_write_mem8() transfer their data through stlink_t::q_buf.
- **Side effects:** the stlink_mwrite_* and stlink_fwrite_* functions start
  the target at the reset vector of the written image when they are done.
- **Threads:** a handle must not be used by more than one thread at a time.
  Different handles can be used in parallel (stlink_probe_usb() does so
  itself). The chip database (init_chipids()) and the logging configuration
  are process wide: set them up once, before opening devices.
- **Output:** the library does not print to stdout or stderr when the
  application has set a log handler (stlink_log_set_handler()) and a progress
  handler for each device (stlink_set_progress_handler()). Exceptions: libusb
  messages on FreeBSD and the deprecated md5_calculate() and stlink_checksum().
- **Errors on the target:** the USB backend does not check whether memory
  reads (and 8 bit writes) succeeded on the target, see stlink_read_mem32().

## Generating this documentation

```sh
cmake -DSTLINK_GENERATE_API_DOCS=ON ..
cmake --build . --target doc-api
```

The HTML pages are written to `doc/api/html` in the build directory.
`-DSTLINK_API_DOCS_INTERNAL=ON` adds the functions that are exported but not
part of the public API ("Internal functions"), for working on the library itself.
Known defects found while documenting are collected on the @ref bug page.
