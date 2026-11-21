#!/usr/bin/env python3
"""Write a firmware image into an ST-LINK/V2 probe through the probe's own DFU bootloader.

Every ST-LINK/V2 (STM32F103 inside) carries a small USB bootloader at 0x08000000. ST's
firmware upgrade tool (STLinkUpgrade / STSW-LINK007) talks to it to replace the probe
application. This module speaks the same protocol, so a probe can be reprogrammed over USB,
without an SWD programmer and without ST's Java tool:

    python stlinkv2_dfu.py firmware.hex              # program the probe (.hex or .bin from 0x08000000)
    python stlinkv2_dfu.py firmware.hex --dry-run    # only show what would be written
    python stlinkv2_dfu.py --list                    # is a probe plugged in? (USB IDs only)
    python stlinkv2_dfu.py --selftest                # check the crypto against known values
    python stlinkv2_dfu.py full.hex --make-swd-image <UID> --out probe.hex
                                                     # signed image for an SWD programmer

What gets written
    The bootloader cannot rewrite itself (it runs from 0x08000000-0x08003BFF), so the tool
    writes the signature block at 0x08003C00 and the application from 0x08004000 on:
    - an image laid out like ST's firmware (an end marker in the last 16 bytes of a 64 or
      128 KB flash): every page from 0x08004000 to the end of that flash, in the same order
      as ST's tool: invalidate the end marker, application pages, signature block, the data
      page below the marker page, and the end marker last;
    - any other image: only the pages up to the end of the image, the vector-table page last
      but erased first (an interrupted install then never starts a half-written application).

Protocol (from a recorded STLinkUpgrade session and the bootloader disassembly)
    16-byte commands on bulk EP 0x02, replies on bulk EP 0x81:
        F5                  current mode        -> 2 bytes, [0] == 0 means DFU
        F3 08               info                -> flash KB (LE16), FF FF, 4 bytes, UID (12 bytes)
        F3 01 blk ck len    DNLOAD; len bytes follow on EP 0x02 in 64-byte packets
                              blk 0: 5 plain bytes, 0x41 + addr (erase page) or 0x21 + addr (set address)
                              blk 2: firmware bytes, encrypted; ck = 16-bit sum of the plain bytes
        F3 03               GETSTATUS           -> status, poll time ms (LE24), state (5 = dnload-idle)
        F3 07               leave DFU and start the application

Keys
    ST's AES-128-ECB with every 32-bit word byte-swapped in key, input and output.
    Per-device key  = AES(key "I am key, wawawa", block)
    Signature       = AES(key "What are you doi", block), stored at 0x08003C00; ST's
                      application compares it with its own computation at start-up
    block           = flash size in KB (LE16), FF FF, the 12-byte UID of the probe's STM32

Needs: pyusb with a libusb-1.0 backend (pip install pyusb libusb-package), pycryptodome or
cryptography, and on Windows the ST-LINK USB driver (WinUSB based, e.g. ST's STSW-LINK009).
"""
import argparse
import os
import struct
import sys
import time

ST_VID, ST_PID = 0x0483, 0x3748     # ST-LINK/V2, in DFU mode and when running ST's application
FLASH_BASE = 0x08000000
PAGE = 0x400                        # STM32F103 medium density: 1 KB flash pages
SIG_ADDR = 0x08003C00               # signature block (first 16 bytes of the page)
APP_ADDR = 0x08004000               # application vector table
IMAGE_SIZE = 0x20000                # largest image handled: 128 KB


def marker_addr(image_kb):
    """Address of the 16-byte end marker in an ST-layout image of image_kb KB (last 16 bytes)."""
    return FLASH_BASE + image_kb * 1024 - 16


def data_page(image_kb):
    """Address of the data page that ST's tool writes right before the end marker."""
    return FLASH_BASE + image_kb * 1024 - 0x800


def marker_page(image_kb):
    """Address of the page holding the end marker (the last page of the flash)."""
    return FLASH_BASE + image_kb * 1024 - 0x400


# ---------------------------------------------------------------------------- errors


class FlashError(Exception):
    """Anything that stops an installation; the message is meant for the user."""


class Cancelled(FlashError):
    """The caller's cancel() returned True; the probe stays in DFU mode."""


class DfuError(Exception):
    """The bootloader reported an error or stopped answering."""


# ---------------------------------------------------------------------------- crypto


def _aes_ecb(key, data, decrypt=False):
    """Plain AES-128-ECB over data (a multiple of 16 bytes) with pycryptodome or cryptography.

    key:     16-byte key
    data:    bytes to encrypt / decrypt
    decrypt: True to decrypt
    returns: the result, same length as data
    """
    try:
        from Crypto.Cipher import AES
        c = AES.new(key, AES.MODE_ECB)
        return c.decrypt(data) if decrypt else c.encrypt(data)
    except ImportError:
        from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
        c = Cipher(algorithms.AES(key), modes.ECB())
        x = c.decryptor() if decrypt else c.encryptor()
        return x.update(data) + x.finalize()


def _wswap(b):
    """Reverse the byte order of every 32-bit word (ST's AES works on big-endian words)."""
    return b"".join(b[i:i + 4][::-1] for i in range(0, len(b), 4))


def st_encrypt(key, data):
    """ST's variant of AES-128-ECB encryption: words byte-swapped in key, input and output."""
    return _wswap(_aes_ecb(_wswap(key), _wswap(data)))


def st_decrypt(key, data):
    """Inverse of st_encrypt()."""
    return _wswap(_aes_ecb(_wswap(key), _wswap(data), decrypt=True))


def id_block(flash_kb, uid):
    """The 16-byte block both keys are derived from.

    flash_kb: flash size the probe's STM32 reports (F_SIZE register, e.g. 64 or 128)
    uid:      the 12-byte unique ID of the probe's STM32 (0x1FFFF7E8)
    """
    return struct.pack("<H", flash_kb) + b"\xff\xff" + uid


def device_key(flash_kb, uid):
    """Key the bootloader of this probe uses to decrypt the firmware blocks."""
    return st_encrypt(b"I am key, wawawa", id_block(flash_kb, uid))


def signature(flash_kb, uid):
    """16-byte signature for 0x08003C00 that ST's application checks for this probe."""
    return st_encrypt(b"What are you doi", id_block(flash_kb, uid))


def selftest():
    """Check the crypto against known-answer values; returns True when everything matches.

    The values for UID 00 01 .. 0B and 128 KB were checked against the blocks ST's own
    upgrade tool sends to a probe.
    """
    uid = bytes(range(12))
    ok = (device_key(128, uid).hex() == "85bce88e7ce98aee27aaf988bd737007" and
          signature(128, uid).hex() == "882af54fe4e1c2de1f68e013034974f7")
    blk = os.urandom(64)
    return ok and st_decrypt(b"k" * 16, st_encrypt(b"k" * 16, blk)) == blk


# ---------------------------------------------------------------------------- image


def load_image(path):
    """Read a .hex (Intel HEX) or .bin (raw, from 0x08000000) file.

    returns: a 128 KB bytearray image of the flash from 0x08000000, 0xFF where the file has no data
    raises:  FlashError for a .bin larger than 128 KB
    """
    mem = bytearray(b"\xff" * IMAGE_SIZE)
    if path.lower().endswith(".hex"):
        base = 0
        with open(path) as f:
            for line in f:
                line = line.strip()
                if not line.startswith(":"):
                    continue
                b = bytes.fromhex(line[1:])
                n, a, t, d = b[0], (b[1] << 8) | b[2], b[3], b[4:4 + b[0]]
                if t == 0:                                  # data
                    off = base + a - FLASH_BASE
                    if 0 <= off and off + n <= len(mem):
                        mem[off:off + n] = d
                elif t == 4:                                # extended linear address
                    base = ((d[0] << 8) | d[1]) << 16
                elif t == 2:                                # extended segment address
                    base = ((d[0] << 8) | d[1]) << 4
    else:
        with open(path, "rb") as f:
            data = f.read()
        if len(data) > len(mem):
            raise FlashError("image larger than 128 KB")
        mem[:len(data)] = data
    return mem


def image_kb(mem):
    """Flash size an image is laid out for: 64 when nothing is above 0x08010000, else 128."""
    return 64 if all(b == 0xFF for b in mem[0x10000:]) else 128


def has_st_layout(mem):
    """True when the image carries an end marker in the last 16 bytes of its 64 / 128 KB flash,
    like ST's own firmware images; such images are written page by page up to the end of the
    flash, in ST's order (see build_plan)."""
    m = marker_addr(image_kb(mem)) - FLASH_BASE
    return any(b != 0xFF for b in mem[m:m + 16])


def image_end(mem):
    """Address just past the last non-erased byte of the image."""
    return FLASH_BASE + len(bytes(mem).rstrip(b"\xff"))


def needed_kb(mem):
    """Flash the image needs, in KB: its layout (64 / 128) for ST-layout images, otherwise the
    bytes up to its end."""
    if has_st_layout(mem):
        return image_kb(mem)
    return (image_end(mem) - FLASH_BASE + 1023) // 1024


def check_image(mem):
    """Raise FlashError unless the image has a plausible vector table at 0x08004000
    (initial stack pointer in RAM, Thumb reset vector inside the image)."""
    sp, rv = struct.unpack_from("<II", mem, APP_ADDR - FLASH_BASE)
    end = FLASH_BASE + image_kb(mem) * 1024
    if not (0x20000000 <= sp <= 0x20005000) or not (rv & 1) or not (APP_ADDR <= rv < end):
        raise FlashError("no application vector table at 0x08004000 - wrong image?")


def check_fits(mem, flash_kb):
    """Raise FlashError if the image needs more flash than the probe's STM32 reports."""
    if flash_kb < 64:
        raise FlashError(f"the probe reports {flash_kb} KB flash; at least 64 KB are needed")
    if needed_kb(mem) > flash_kb:
        raise FlashError(f"the image needs {needed_kb(mem)} KB of flash, the probe reports {flash_kb} KB")


# ---------------------------------------------------------------------------- plan


def page_ops(mem, page, data_from=None, data_to=None):
    """Operations for one page: erase it, then set the address and send the non-erased part.

    mem:       image from 0x08000000
    page:      page address
    data_from: first address to write (default: the page start)
    data_to:   address after the last byte to write (default: the page end)
    returns:   list of ("erase", addr), ("addr", addr), ("data", addr, bytes) tuples; the data
               is padded to 16 bytes (one AES block) and only sent when it is not all 0xFF
    """
    lo = page if data_from is None else data_from
    hi = page + PAGE if data_to is None else data_to
    chunk = bytes(mem[lo - FLASH_BASE:hi - FLASH_BASE])
    ops = [("erase", page)]
    trimmed = chunk.rstrip(b"\xff")
    if trimmed:
        n = (len(trimmed) + 15) // 16 * 16
        ops += [("addr", lo), ("data", lo, chunk[:n])]
    return ops


def build_plan(mem, flash_kb, uid):
    """Everything the bootloader has to do, in order.

    mem:      image from 0x08000000 (its signature block is replaced)
    flash_kb: flash size the probe reports
    uid:      12-byte UID of the probe's STM32
    returns:  (ops, img): the operations (see page_ops) and the image as written, with the
              signature for this probe at 0x08003C00

    ST-layout image (has_st_layout): the same order as ST's tool - invalidate the end marker,
    write the application pages, the signature block, the data page and the end marker last.
    Other images: erase the vector-table page first, write the remaining pages up to the end of
    the image, then the vector-table page and the signature page. Nothing above the image is
    touched.
    """
    kb = image_kb(mem)
    img = bytearray(mem)
    img[SIG_ADDR - FLASH_BASE:SIG_ADDR - FLASH_BASE + 16] = signature(flash_kb, uid)
    if not has_st_layout(mem):
        end = (image_end(img) + PAGE - 1) & ~(PAGE - 1)
        if end > FLASH_BASE + flash_kb * 1024:
            raise FlashError(f"the image needs {needed_kb(img)} KB of flash, the probe reports {flash_kb} KB")
        ops = [("erase", APP_ADDR)]
        for page in range(APP_ADDR + PAGE, end, PAGE):
            ops += page_ops(img, page)
        ops += page_ops(img, APP_ADDR)[1:]                  # already erased above
        ops += page_ops(img, SIG_ADDR)
        return ops, img
    marker, dp, mp = marker_addr(kb), data_page(kb), marker_page(kb)
    ops = [("erase", mp), ("addr", marker), ("data", marker, b"\xff" * 16)]
    for page in range(APP_ADDR, dp, PAGE):
        ops += page_ops(img, page)
    ops += page_ops(img, SIG_ADDR)
    ops += page_ops(img, dp)
    ops += page_ops(img, mp, data_to=marker)
    ops += [("addr", marker), ("data", marker, bytes(img[marker - FLASH_BASE:marker - FLASH_BASE + 16]))]
    return ops, img


# ---------------------------------------------------------------------------- SWD image


def parse_uid(text):
    """Parse a probe UID typed by the user (24 hex digits; spaces, '-' and ':' ignored)."""
    t = text.strip().replace("-", "").replace(" ", "").replace(":", "")
    try:
        uid = bytes.fromhex(t)
    except ValueError:
        uid = b""
    if len(uid) != 12:
        raise FlashError("UID must be 24 hex digits (the 12 bytes at 0x1FFFF7E8 of the probe's STM32)")
    return uid


def make_swd_image(mem, uid, flash_kb=None):
    """Full image (bootloader included) for an SWD programmer, signed for one probe.

    mem:      full image from 0x08000000, bootloader included
    uid:      12-byte UID of the probe's STM32
    flash_kb: flash size its STM32 reports; default: the image's layout (64 or 128)
    returns:  the image with the signature for this probe at 0x08003C00
    """
    flash_kb = flash_kb or image_kb(mem)
    if bytes(mem[:8]) == b"\xff" * 8:
        raise FlashError("this file has no bootloader at 0x08000000; an SWD image needs the full image")
    check_image(mem)
    img = bytearray(mem)
    img[SIG_ADDR - FLASH_BASE:SIG_ADDR - FLASH_BASE + 16] = signature(flash_kb, uid)
    return img


def write_hex(path, mem, base=FLASH_BASE):
    """Write the non-erased 16-byte rows of mem as an Intel HEX file starting at base."""
    out = []

    def rec(t, a, d):
        b = bytes([len(d), (a >> 8) & 0xFF, a & 0xFF, t]) + d
        out.append(":" + b.hex().upper() + "%02X" % ((-sum(b)) & 0xFF))
    upper = None
    for off in range(0, len(mem), 16):
        row = bytes(mem[off:off + 16])
        if row == b"\xff" * len(row):
            continue
        a = base + off
        if a >> 16 != upper:
            upper = a >> 16
            rec(4, 0, bytes([upper >> 8, upper & 0xFF]))
        rec(0, a & 0xFFFF, row)
    rec(1, 0, b"")
    with open(path, "w", newline="\r\n") as f:
        f.write("\n".join(out) + "\n")


# ---------------------------------------------------------------------------- protocol


class StlinkDfu:
    """The bootloader's DFU protocol on top of a transport.

    io: any object with out(ep, data) and inp(ep, n) -> bytes (UsbIo, or a model in the tests)
    """

    def __init__(self, io):
        self.io = io

    def cmd(self, b, reply=0):
        """Send a 16-byte command (b padded with zeros); read reply bytes if reply > 0."""
        self.io.out(0x02, bytes(b).ljust(16, b"\0"))
        return self.io.inp(0x81, reply) if reply else b""

    def mode(self):
        """Current mode; byte 0 == 0 means the bootloader (DFU) is running."""
        return self.cmd([0xF5], 2)

    def info(self):
        """returns: (flash size in KB, 12-byte UID) of the probe's STM32."""
        r = self.cmd([0xF3, 0x08, 0x02], 20)
        return struct.unpack_from("<H", r, 0)[0], bytes(r[8:20])

    def status(self):
        """returns: (status, poll time in ms, state); status != 0 is an error."""
        r = self.cmd([0xF3, 0x03, 0, 0, 0, 0, 6, 0], 6)
        return r[0], r[1] | (r[2] << 8) | (r[3] << 16), r[4]

    def wait_idle(self):
        """Poll GETSTATUS until the last download is done; raise DfuError on an error status."""
        for _ in range(200):
            st, poll, state = self.status()
            if st != 0:
                raise DfuError(f"bootloader reports error status {st}" +
                               (" (checksum mismatch after write)" if st == 3 else ""))
            if state in (2, 5):
                return
            time.sleep(max(poll, 1) / 1000.0)
        raise DfuError("bootloader stays busy")

    def dnload(self, block, payload, checksum):
        """One DNLOAD: header command, payload in 64-byte packets, then wait until done.

        block:    0 for erase / set-address commands, 2 for firmware data
        payload:  bytes as sent (firmware data already encrypted)
        checksum: 16-bit sum of the plain bytes
        """
        self.cmd([0xF3, 0x01, block & 0xFF, block >> 8, checksum & 0xFF, checksum >> 8,
                  len(payload) & 0xFF, len(payload) >> 8])
        for i in range(0, len(payload), 64):
            self.io.out(0x02, payload[i:i + 64])
        self.wait_idle()

    def block0(self, code, addr):
        """Block-0 command: 0x41 = erase the page at addr, 0x21 = set the write address."""
        p = bytes([code]) + struct.pack("<I", addr)
        self.dnload(0, p, sum(p))

    def run(self, ops, key, progress=None):
        """Execute a plan from build_plan() with the device key; progress(done, total) per step."""
        for i, op in enumerate(ops):
            if op[0] == "erase":
                self.block0(0x41, op[1])
            elif op[0] == "addr":
                self.block0(0x21, op[1])
            else:
                plain = op[2]
                self.dnload(2, st_encrypt(key, plain), sum(plain) & 0xFFFF)
            if progress:
                progress(i + 1, len(ops))

    def exit(self):
        """Leave DFU mode; the bootloader starts the application."""
        self.cmd([0xF3, 0x07])


# ---------------------------------------------------------------------------- USB


def usb_backend():
    """libusb-1.0 backend for pyusb: libusb-package's bundled library first, then the system's."""
    import usb.backend.libusb1 as l1
    be = None
    try:
        import libusb_package
        be = libusb_package.get_libusb1_backend()
    except ImportError:
        pass
    if be is None:
        be = l1.get_backend()
    if be is None:
        raise FlashError("no libusb-1.0 backend found (pip install libusb-package)")
    return be


class UsbIo:
    """Bulk transfers to one pyusb device (the transport for StlinkDfu)."""

    def __init__(self, dev):
        self.dev = dev
        try:
            dev.set_configuration()
        except Exception:
            pass                                            # already configured

    def out(self, ep, data):
        """Write data to endpoint ep."""
        self.dev.write(ep, data, timeout=3000)

    def inp(self, ep, n):
        """Read one packet from endpoint ep and return its first n bytes."""
        return bytes(self.dev.read(ep, 64, timeout=3000))[:n]

    def close(self):
        """Release the device."""
        try:
            import usb.util
            usb.util.dispose_resources(self.dev)
        except Exception:
            pass


def find(be, vid, pid):
    """First USB device with this VID:PID, or None."""
    import usb.core
    return usb.core.find(idVendor=vid, idProduct=pid, backend=be)


def probe_present(be):
    """True when an ST-LINK/V2 is plugged in (looks at the USB IDs only, sends nothing)."""
    return find(be, ST_VID, ST_PID) is not None


def _sleep(seconds, cancel):
    """Sleep, raising Cancelled as soon as cancel() returns True."""
    t = time.time() + seconds
    while time.time() < t:
        if cancel and cancel():
            raise Cancelled("cancelled")
        time.sleep(0.05)


def open_dfu(be, log=print, cancel=None, replug_seconds=120):
    """Return a StlinkDfu talking to the bootloader.

    A probe that runs its application enumerates with the same USB IDs; the bootloader only
    runs right after power-up, so the user is asked to unplug and replug the probe.

    be:             pyusb backend (usb_backend())
    log:            function(str) for messages to the user
    cancel:         function() -> bool, True to give up
    replug_seconds: how long to wait for the replugged probe
    """
    d = find(be, ST_VID, ST_PID)
    if d is not None:
        io = UsbIo(d)
        try:
            dfu = StlinkDfu(io)
            if dfu.mode()[0] == 0:
                return dfu
        except Exception:
            pass
        io.close()
        log("The probe is running its application, not the bootloader.")
    log(f"Unplug the probe and plug it back in now (waiting up to {replug_seconds} s) ...")
    t = time.time() + replug_seconds
    seen_gone = d is None
    while time.time() < t:
        d = find(be, ST_VID, ST_PID)
        if d is None:
            seen_gone = True
        elif seen_gone:
            io = UsbIo(d)
            try:
                dfu = StlinkDfu(io)
                if dfu.mode()[0] == 0:
                    return dfu
            except Exception:
                pass
            io.close()
        _sleep(0.05, cancel)
    raise FlashError("no ST-LINK/V2 in DFU mode found")


def install(mem, log=print, progress=None, cancel=None, be=None):
    """Write an image into the probe through its bootloader.

    mem:      image from 0x08000000 (load_image)
    log:      function(str) for messages to the user
    progress: function(done, total), called after every bootloader operation
    cancel:   function() -> bool; checked between operations (the probe then stays in DFU mode
              and the next run finishes the job)
    be:       pyusb backend (default: usb_backend())
    returns:  dict with the probe's "uid" (hex) and "flash_kb"
    """
    check_image(mem)
    be = be or usb_backend()
    dfu = open_dfu(be, log, cancel)
    flash_kb, uid = dfu.info()
    log(f"Bootloader found: flash {flash_kb} KB, UID {uid.hex()}")
    check_fits(mem, flash_kb)
    ops, _ = build_plan(mem, flash_kb, uid)
    key = device_key(flash_kb, uid)
    n_pages = sum(1 for o in ops if o[0] == "erase")
    log(f"Writing {n_pages} pages, signature {signature(flash_kb, uid).hex()} ...")

    def step(i, n):
        if cancel and cancel() and i < n:
            raise Cancelled("cancelled while writing - the probe stays in DFU mode; run again")
        if progress:
            progress(i, n)
    try:
        dfu.run(ops, key, step)
    except DfuError as e:
        raise FlashError(str(e) + " - the probe stays in DFU mode; run again")
    dfu.exit()
    dfu.io.close()
    log("Done. The probe leaves DFU mode and starts the new firmware.")
    return {"uid": uid.hex(), "flash_kb": flash_kb}


# ---------------------------------------------------------------------------- command line


def main(argv=None):
    """Command-line entry point; returns the process exit code."""
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("image", nargs="?", help="firmware image (.bin from 0x08000000, or .hex)")
    ap.add_argument("--dry-run", action="store_true", help="only show what would be written")
    ap.add_argument("--uid", help="probe UID (24 hex digits) for --dry-run")
    ap.add_argument("--flash-kb", type=int,
                    help="probe flash size for --dry-run / --make-swd-image (default: the image's layout, 64 or 128)")
    ap.add_argument("--selftest", action="store_true", help="check the crypto against known values")
    ap.add_argument("--list", action="store_true", help="show whether an ST-LINK/V2 is plugged in")
    ap.add_argument("--make-swd-image", metavar="UID",
                    help="write a full image signed for this probe UID (for an SWD programmer) "
                         "instead of programming over USB; see --out")
    ap.add_argument("--out", help="output .hex/.bin for --make-swd-image")
    a = ap.parse_args(argv)

    if not selftest():
        raise SystemExit("crypto self-test FAILED")
    if a.selftest:
        print("crypto self-test OK")
        return 0
    if a.list:
        print("ST-LINK/V2 (0483:3748) found" if probe_present(usb_backend()) else "no ST-LINK/V2 found")
        return 0
    if not a.image:
        ap.error("image required")
    try:
        mem = load_image(a.image)
        check_image(mem)
        a.flash_kb = a.flash_kb or image_kb(mem)
        check_fits(mem, a.flash_kb)
        if a.make_swd_image:
            uid = parse_uid(a.make_swd_image)
            img = make_swd_image(mem, uid, a.flash_kb)
            out = a.out or f"stlinkv2_{uid.hex()}.hex"
            if out.lower().endswith(".bin"):
                with open(out, "wb") as f:
                    f.write(bytes(img))
            else:
                write_hex(out, img)
            print(f"wrote {out}: full image with signature {signature(a.flash_kb, uid).hex()} "
                  f"for UID {uid.hex()} - program it at 0x08000000 over SWD (mass erase first)")
            return 0
        if a.dry_run:
            uid = parse_uid(a.uid) if a.uid else bytes(12)
            ops, _ = build_plan(mem, a.flash_kb, uid)
            n_e = sum(1 for o in ops if o[0] == "erase")
            n_d = [o for o in ops if o[0] == "data"]
            print(f"{n_e} page erases, {sum(len(o[2]) for o in n_d)} bytes in {len(n_d)} blocks")
            print(f"signature for UID {uid.hex()}: {signature(a.flash_kb, uid).hex()}")
            return 0
    except FlashError as e:
        print("ERROR:", e)
        return 1

    def progress(i, n):
        sys.stdout.write(f"\rwriting {i * 100 // n:3d}%")
        sys.stdout.flush()
    try:
        install(mem, progress=progress)
    except FlashError as e:
        print("\nERROR:", e)
        return 1
    print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
