# stlinkv2-dfu: reprogram an ST-LINK/V2 through its own USB bootloader

Every ST-LINK/V2 probe (an STM32F103 inside) has a small USB bootloader at `0x08000000`. ST's
firmware upgrade tool (STLinkUpgrade, STSW-LINK007) uses it to replace the probe application.
`stlinkv2_dfu.py` speaks the same protocol, so a firmware image can be written into the probe
over USB - without an SWD programmer and without ST's Java tool. It also makes images for an SWD
programmer that carry the right signature for one particular probe.

| File | |
|---|---|
| `stlinkv2_dfu.py` | protocol, crypto, image handling and the command line |
| `stlinkv2_dfu_gui.py` | a window (tkinter) on top of it; also builds into a single Windows `.exe` |
| `test_stlinkv2_dfu.py` | offline tests against a model of the bootloader (no probe needed) |
| `requirements.txt` | Python packages |

No firmware is included: the tool writes an image you provide (`.hex`, or `.bin` from
`0x08000000`), e.g. your own build of a probe application.

## Install

```sh
pip install -r requirements.txt          # pyusb, libusb-package, pycryptodome
```

- **Windows:** the probe needs ST's ST-LINK USB driver (STSW-LINK009, also installed with
  STM32CubeProgrammer / STM32CubeIDE).
- **Linux:** the udev rules shipped with stlink (`config/udev/rules.d/`) give access to `0483:3748`.

## Use

```sh
python stlinkv2_dfu.py firmware.hex              # program the probe
python stlinkv2_dfu.py firmware.hex --dry-run    # show the plan only (page erases, bytes, signature)
python stlinkv2_dfu.py --list                    # is an ST-LINK/V2 plugged in? (USB IDs only)
python stlinkv2_dfu.py --selftest                # crypto known-answer test
python stlinkv2_dfu_gui.py                       # the same with a window
```

The bootloader only runs right after power-up. If the probe is running its application, the tool
asks you to unplug it and plug it back in, then continues. **Cancel** (or an interrupted
transfer) leaves the probe in DFU mode; running the tool again finishes the job.

### Signed image for an SWD programmer

ST's probe application checks a 16-byte signature at `0x08003C00` that depends on the UID of the
probe's STM32. Over USB the tool writes the right one by itself. For an SWD programmer, make a
copy of a full image (bootloader included) for one probe:

```sh
python stlinkv2_dfu.py full.hex --make-swd-image <UID> --out probe.hex
```

`<UID>` is the 24 hex digits at `0x1FFFF7E8` of the probe's STM32 (read them with any SWD
programmer). The result differs from the input only in those 16 bytes. Mass-erase the probe's
STM32 first, then write the file at `0x08000000`. Add `--flash-kb 64` or `--flash-kb 128` when
the flash size the STM32 reports differs from the image's layout.

### Windows executable

```bat
pip install pyinstaller
python -m PyInstaller --onefile --windowed --name stlinkv2_dfu --collect-binaries libusb_package stlinkv2_dfu_gui.py
```

`dist\stlinkv2_dfu.exe` opens the window; `stlinkv2_dfu.exe --cli <arguments>` (or any argument
starting with `-`) runs the command line in the calling console.

## What gets written

The bootloader (`0x08000000`-`0x08003BFF`) cannot rewrite itself and is never touched. The tool
writes the signature page at `0x08003C00` and the application from `0x08004000`:

- **Image laid out like ST's firmware** (an end marker in the last 16 bytes of a 64 KB or 128 KB
  flash): every page from `0x08004000` to the end of that flash, in the same order as ST's tool -
  invalidate the end marker, write the application pages, the signature block, the data page
  just below the marker page, and the end marker last.
- **Any other image:** only the pages up to the end of the image. The vector-table page is
  erased first and written last, so an interrupted installation never starts a half-written
  application. Nothing above the image is touched.

The image must fit the flash size the bootloader reports (64 or 128 KB parts).

## Protocol

16-byte commands on bulk endpoint `0x02`, replies on `0x81`:

| Command | Meaning |
|---|---|
| `F5` | current mode - 2 bytes, byte 0 = `0` means the bootloader (DFU) runs |
| `F3 08` | info - flash size in KB (LE16), `FF FF`, 4 bytes, 12-byte UID |
| `F3 01 blk ck len` | DNLOAD; `len` bytes follow on `0x02` in 64-byte packets. Block 0: 5 plain bytes, `0x41` + address = erase page, `0x21` + address = set address. Block 2: firmware bytes, encrypted; `ck` = 16-bit sum of the plain bytes, checked by the bootloader after writing |
| `F3 03` | GETSTATUS - status (`0` ok, `3` checksum mismatch), poll time in ms (LE24), state (`5` = download idle) |
| `F3 07` | leave DFU and start the application |

Encryption is ST's variant of AES-128-ECB: every 32-bit word is byte-swapped in key, input and
output. With `block` = flash size in KB (LE16), `FF FF`, UID (12 bytes):

- per-device firmware key = AES(key `"I am key, wawawa"`, `block`)
- signature at `0x08003C00` = AES(key `"What are you doi"`, `block`)

The command sequence and the encrypted blocks were checked against a recorded session of ST's
upgrade tool: for the same firmware and probe, every data block is byte-identical.

## Tests

```sh
python test_stlinkv2_dfu.py
```

The tests run the complete protocol exchange of an installation against `FakeBootloader`, a
model of the bootloader's DFU that decrypts every block with the device key, only programs erased
cells and verifies the checksum, then compare the resulting flash for synthetic images (ST layout
at 128 KB and 64 KB, application-only, another UID) and check the crypto, HEX round trip and the
SWD image.
