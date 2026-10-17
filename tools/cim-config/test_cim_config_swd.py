# Copyright (c) 2026, The CIM Contributors
# SPDX-License-Identifier: BSD-3-Clause
"""Tests for the image format, against an image written by the firmware."""

import struct
from pathlib import Path

import cim_config_swd
from cim_config_swd import SECTOR_SIZE, Image, newest, next_image

# sector B after the config_counter hardware test: boot counter (key 0x8000) = 4, sequence 4
FIRMWARE_IMAGE = (struct.pack("<IIII", 0x434D4943, 0x00070001, 4, 0x2E95C4F0)
                  + bytes([0x00, 0x80, 0x04, 0x04, 0x00, 0x00, 0x00]))


def sector(data):
    return data + b"\xff" * (SECTOR_SIZE - len(data))


def test_parse_firmware_image():
    img = Image.parse(sector(FIRMWARE_IMAGE))
    assert img.seq == 4
    assert img.records == {0x8000: (4).to_bytes(4, "little")}


def test_encode_matches_firmware():
    img = Image(4, {0x8000: (4).to_bytes(4, "little")})
    assert img.encode() == sector(FIRMWARE_IMAGE)


def test_invalid_sectors():
    assert Image.parse(b"\xff" * SECTOR_SIZE) is None
    corrupt = bytearray(sector(FIRMWARE_IMAGE))
    corrupt[17] ^= 1
    assert Image.parse(bytes(corrupt)) is None


def test_newest_with_wraparound():
    assert newest([None, None]) is None
    assert newest([Image(3), Image(4)]) == 1
    assert newest([Image(5), Image(4)]) == 0
    assert newest([Image(0xFFFFFFFF), Image(0)]) == 1
    assert newest([Image(4), None]) == 0


def test_next_image_wraps_the_sequence():
    assert next_image([None, None], None).seq == 1
    img = next_image([Image(7, {1: b"a"}), None], 0)
    assert img.seq == 8 and img.records == {1: b"a"}
    img = next_image([None, Image(0xFFFFFFFF)], 1)
    assert img.seq == 0
    assert newest([None, img]) == 1
    assert newest([Image(0xFFFFFFFF), img]) == 1
    img.encode()  # packs without an overflow error


def test_read_restarts_the_board(monkeypatch):
    commands = []
    ocd = cim_config_swd.OpenOcd("cmsis-dap", "rp2040", 5000)

    def run(*args):
        commands.extend(args)
        (Path(args[2].split()[1])).write_bytes(b"\xff" * 8)

    monkeypatch.setattr(ocd, "run", run)
    ocd.read(0x10000000, 8)
    assert commands[-1] == "reset run"
