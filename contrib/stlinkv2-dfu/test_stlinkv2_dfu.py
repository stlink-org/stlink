#!/usr/bin/env python3
"""Offline tests for stlinkv2_dfu.py: no probe and no firmware files needed.

    python test_stlinkv2_dfu.py        (or: python -m unittest test_stlinkv2_dfu)

A model of the ST-LINK/V2 bootloader's DFU (FakeBootloader) receives the complete protocol
exchange of a real installation - commands, erase / set-address blocks, encrypted firmware
blocks with checksums - and keeps a flash image; the tests compare that flash with the
expected result for synthetic images.
"""
import os
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import stlinkv2_dfu as dfu  # noqa: E402

UID = bytes.fromhex("0102030405060708090a0b0c")


class FakeBootloader:
    """The bootloader's side of the protocol, as far as the tool uses it.

    flash: initial flash content from 0x08000000 (padded with 0xFF to kb KB)
    uid:   12-byte UID it reports
    kb:    flash size it reports
    The model enforces what the real bootloader does: every data block is decrypted with the
    device key, may only program erased cells, and its checksum is verified after writing
    (status 3 otherwise).
    """

    def __init__(self, flash, uid, kb):
        flash = bytes(flash[:kb * 1024])
        self.flash = bytearray(flash) + b"\xff" * (kb * 1024 - len(flash))
        self.uid, self.kb = uid, kb
        self.key = dfu.device_key(kb, uid)
        self.addr, self.state, self.status = 0, 2, 0
        self.pending, self.reply = None, b""
        self.erased, self.exited = [], False

    def out(self, ep, d):
        d = bytes(d)
        if self.pending:                                    # payload packets of a DNLOAD
            blk, ck, ln, buf = self.pending
            buf += d
            if len(buf) < ln:
                self.pending = (blk, ck, ln, buf)
                return
            self.pending = None
            self._download(blk, ck, buf[:ln])
            return
        if d[0] == 0xF5:
            self.reply = bytes([0, 1])                      # DFU mode
        elif d[:2] == b"\xf3\x08":
            self.reply = struct.pack("<H", self.kb) + b"\xff\xff\x4d\xff\xff\xff" + self.uid
        elif d[:2] == b"\xf3\x03":
            self.reply = bytes([self.status, 0x50, 0, 0, self.state, 0])
        elif d[:2] == b"\xf3\x01":
            self.pending = (d[2] | d[3] << 8, d[4] | d[5] << 8, d[6] | d[7] << 8, b"")
        elif d[:2] == b"\xf3\x07":
            self.exited = True

    def _download(self, blk, ck, data):
        self.state = 5
        if blk == 0:
            if sum(data) & 0xFFFF != ck:
                self.status = 3
                return
            a = int.from_bytes(data[1:5], "little")
            if data[0] == 0x41:                             # erase page
                o = (a & ~0x3FF) - dfu.FLASH_BASE
                self.flash[o:o + 0x400] = b"\xff" * 0x400
                self.erased.append(a & ~0x3FF)
            elif data[0] == 0x21:                           # set address
                self.addr = a
            return
        plain = dfu.st_decrypt(self.key, data)
        o = self.addr - dfu.FLASH_BASE
        for k in range(0, len(plain), 4):
            if self.flash[o + k:o + k + 4] != b"\xff\xff\xff\xff":
                self.status = 3                             # programming a non-erased word
                return
            self.flash[o + k:o + k + 4] = plain[k:k + 4]
        if sum(self.flash[o:o + len(plain)]) & 0xFFFF != ck:
            self.status = 3

    def inp(self, ep, n):
        r, self.reply = self.reply, b""
        return r[:n]


def make_image(kb, st_layout, app_size=0x6000, seed=1):
    """Synthetic firmware image.

    kb:        flash layout, 64 or 128
    st_layout: True for an image like ST's (data page + end marker at the end of the flash)
    app_size:  bytes of application from 0x08004000
    returns:   bytearray image from 0x08000000 with a fake bootloader and a valid vector table
    """
    rnd = __import__("random").Random(seed)
    mem = bytearray(b"\xff" * dfu.IMAGE_SIZE)
    mem[0:0x3C00] = bytes(rnd.getrandbits(8) for _ in range(0x3C00))       # "bootloader"
    app = bytearray(rnd.getrandbits(8) for _ in range(app_size))
    struct.pack_into("<II", app, 0, 0x20005000, dfu.APP_ADDR + 0x101)       # SP, reset vector
    mem[0x4000:0x4000 + app_size] = app
    if st_layout:
        end = kb * 1024
        mem[end - 0x800:end - 0x800 + 16] = bytes(range(16))               # data page
        mem[end - 16:end] = b"\x11" * 12 + b"\xd3\x27\x00\xa5"             # end marker
    elif kb == 128:
        mem[0x10000] = 0                                    # something above 64 KB
    return mem


def upload(mem, kb, uid=UID, initial=None):
    """Run the tool's plan against a FakeBootloader; returns (device, image as written)."""
    dev = FakeBootloader(initial if initial is not None else b"\x00" * (kb * 1024), uid, kb)
    link = dfu.StlinkDfu(dev)
    flash_kb, got_uid = link.info()
    ops, img = dfu.build_plan(mem, flash_kb, got_uid)
    link.run(ops, dfu.device_key(flash_kb, got_uid))
    link.exit()
    return dev, img


class Crypto(unittest.TestCase):
    def test_known_answers(self):
        self.assertTrue(dfu.selftest())

    def test_signature_depends_on_uid_and_size(self):
        self.assertNotEqual(dfu.signature(128, UID), dfu.signature(64, UID))
        self.assertNotEqual(dfu.signature(128, UID), dfu.signature(128, bytes(12)))


class Image(unittest.TestCase):
    def test_layout_detection(self):
        self.assertTrue(dfu.has_st_layout(make_image(128, True)))
        self.assertTrue(dfu.has_st_layout(make_image(64, True)))
        self.assertFalse(dfu.has_st_layout(make_image(64, False)))
        self.assertEqual(dfu.image_kb(make_image(64, True)), 64)
        self.assertEqual(dfu.image_kb(make_image(128, True)), 128)
        self.assertEqual(dfu.needed_kb(make_image(64, False, app_size=0x2000)), 24)

    def test_fits(self):
        dfu.check_fits(make_image(64, True), 128)
        with self.assertRaises(dfu.FlashError):
            dfu.check_fits(make_image(128, True), 64)

    def test_bad_vector_table(self):
        mem = make_image(128, True)
        mem[0x4000:0x4008] = b"\xff" * 8
        with self.assertRaises(dfu.FlashError):
            dfu.check_image(mem)

    def test_hex_round_trip(self):
        mem = make_image(128, True)
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "x.hex")
            dfu.write_hex(p, mem)
            self.assertEqual(dfu.load_image(p), mem)


class Upload(unittest.TestCase):
    def check_flash(self, dev, img, mem, kb, last):
        """Bootloader untouched, signature for this probe, application = image up to last."""
        self.assertEqual(dev.status, 0)
        self.assertTrue(dev.exited)
        self.assertEqual(bytes(dev.flash[0x3C00:0x3C10]), dfu.signature(kb, UID))
        self.assertEqual(bytes(dev.flash[0x3C10:last]), bytes(mem[0x3C10:last]))
        self.assertNotIn(0x08000000, dev.erased)

    def test_st_layout_128(self):
        mem = make_image(128, True)
        dev, img = upload(mem, 128)
        self.check_flash(dev, img, mem, 128, 0x20000)
        self.assertEqual(dev.erased[0], dfu.marker_page(128))           # marker invalidated first

    def test_st_layout_64(self):
        mem = make_image(64, True)
        dev, img = upload(mem, 64)
        self.check_flash(dev, img, mem, 64, 0x10000)

    def test_application_only(self):
        mem = make_image(64, False, app_size=0x2345)
        old = bytes(range(256)) * 256                       # old flash content, 64 KB
        dev, img = upload(mem, 64, initial=old)
        end = 0x4000 + ((0x2345 + 0x3FF) & ~0x3FF)
        self.check_flash(dev, img, mem, 64, end)
        self.assertEqual(bytes(dev.flash[end:]), old[end:])             # nothing above is touched
        self.assertEqual(dev.erased[0], dfu.APP_ADDR)                   # vector table erased first

    def test_other_uid(self):
        uid2 = bytes(range(20, 32))
        mem = make_image(128, True)
        dev, _ = upload(mem, 128, uid=uid2)
        self.assertEqual(dev.status, 0)
        self.assertEqual(bytes(dev.flash[0x3C00:0x3C10]), dfu.signature(128, uid2))

    def test_swd_image(self):
        mem = make_image(128, True)
        img = dfu.make_swd_image(mem, UID)
        diff = [i for i in range(len(mem)) if img[i] != mem[i]]
        self.assertTrue(all(0x3C00 <= i < 0x3C10 for i in diff))
        self.assertEqual(bytes(img[0x3C00:0x3C10]), dfu.signature(128, UID))


if __name__ == "__main__":
    unittest.main(verbosity=2)
