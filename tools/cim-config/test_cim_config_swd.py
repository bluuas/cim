# Copyright (c) 2026, The CIM Contributors
# SPDX-License-Identifier: BSD-3-Clause
"""Tests for the image format, against an image written by the firmware."""

import struct

from cim_config_swd import SECTOR_SIZE, Image, newest

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
