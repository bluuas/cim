#!/usr/bin/env python3
# Copyright (c) 2026, The CIM Contributors
# SPDX-License-Identifier: BSD-3-Clause
"""Read and change the configuration of a CIM over SWD (debug probe + OpenOCD).

Fallback for commissioning when the board has no working firmware with USB
commissioning (see firmware/commission). Implements the same image format as
firmware/config/src/config.c and writes the new image to the other sector, so
a failed write keeps the old configuration.

    cim_config_swd.py show
    cim_config_swd.py set-address 5
    cim_config_swd.py set-name pedalbox
"""

import argparse
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

MAGIC = 0x434D4943  # "CIMC"
FORMAT_VERSION = 1
HEADER_SIZE = 16
SECTOR_SIZE = 4096
MAX_PAYLOAD = SECTOR_SIZE - HEADER_SIZE
XIP_BASE = 0x10000000

KEY_ADDRESS = 0x0001
KEY_NAME = 0x0002
KEY_NAMES = {KEY_ADDRESS: "address", KEY_NAME: "name"}


class Image:
    """One configuration image: sequence number and records {key: bytes}."""

    def __init__(self, seq=0, records=None):
        self.seq = seq
        self.records = dict(records or {})

    @staticmethod
    def parse(sector):
        """Parse a sector. Returns None if it holds no valid image."""
        if len(sector) < HEADER_SIZE:
            return None
        magic, version, length, seq, crc = struct.unpack_from("<IHHII", sector)
        if magic != MAGIC or version != FORMAT_VERSION or length > MAX_PAYLOAD:
            return None
        payload = sector[HEADER_SIZE:HEADER_SIZE + length]
        if zlib.crc32(payload, zlib.crc32(sector[:12])) != crc:
            return None
        records, pos = {}, 0
        while pos + 3 <= length:
            key, n = struct.unpack_from("<HB", payload, pos)
            records[key] = bytes(payload[pos + 3:pos + 3 + n])
            pos += 3 + n
        return Image(seq, records)

    def encode(self):
        """Encode as a full sector (padded with 0xFF)."""
        payload = b"".join(struct.pack("<HB", k, len(v)) + v for k, v in self.records.items())
        if len(payload) > MAX_PAYLOAD:
            raise ValueError("configuration does not fit into one sector")
        header = struct.pack("<IHHI", MAGIC, FORMAT_VERSION, len(payload), self.seq)
        crc = zlib.crc32(payload, zlib.crc32(header))
        data = header + struct.pack("<I", crc) + payload
        return data + b"\xff" * (SECTOR_SIZE - len(data))


def newest(images):
    """Index of the newest valid image (sequence numbers wrap around), or None."""
    best = None
    for i, img in enumerate(images):
        if img is None:
            continue
        if best is None or ((img.seq - images[best].seq) & 0xFFFFFFFF) - (1 << 31) < 0 and img.seq != images[best].seq:
            best = i
    return best


def describe(key, value):
    name = KEY_NAMES.get(key, f"0x{key:04X}")
    if key == KEY_ADDRESS and len(value) == 1:
        return f"{name} = {value[0]}"
    if key == KEY_NAME:
        return f"{name} = {value.decode(errors='replace')}"
    return f"{name} = {value.hex()}"


class OpenOcd:
    def __init__(self, interface, target, speed):
        self.base = ["openocd", "-f", f"interface/{interface}.cfg", "-f", f"target/{target}.cfg",
                     "-c", f"adapter speed {speed}"]

    def run(self, *commands):
        args = self.base + [a for c in commands for a in ("-c", c)] + ["-c", "shutdown"]
        result = subprocess.run(args, capture_output=True, text=True)
        if result.returncode != 0:
            sys.exit(f"openocd failed:\n{result.stderr[-2000:]}")

    def read(self, address, size):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "dump.bin"
            self.run("init", "reset halt", f"dump_image {path} 0x{address:08X} {size}")
            return path.read_bytes()

    def write_sector(self, address, data):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "sector.bin"
            path.write_bytes(data)
            self.run("init", "reset halt", f"flash write_image erase {path} 0x{address:08X} bin",
                     f"verify_image {path} 0x{address:08X} bin", "reset run")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--flash-size", type=lambda s: int(s, 0), default=2 * 1024 * 1024, help="default: 2 MB")
    p.add_argument("--interface", default="cmsis-dap", help="OpenOCD interface (default: cmsis-dap)")
    p.add_argument("--target", default="rp2040", help="OpenOCD target (default: rp2040)")
    p.add_argument("--speed", type=int, default=5000, help="SWD speed in kHz (default: 5000)")
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("show", help="print the configuration")
    a = sub.add_parser("set-address", help="set the node address (1..239)")
    a.add_argument("address", type=lambda s: int(s, 0))
    n = sub.add_parser("set-name", help="set the node name (1..32 printable characters)")
    n.add_argument("name")
    args = p.parse_args()

    ocd = OpenOcd(args.interface, args.target, args.speed)
    base = XIP_BASE + args.flash_size - 2 * SECTOR_SIZE
    data = ocd.read(base, 2 * SECTOR_SIZE)
    images = [Image.parse(data[:SECTOR_SIZE]), Image.parse(data[SECTOR_SIZE:])]
    cur = newest(images)

    if args.cmd == "show":
        if cur is None:
            print("no configuration stored")
            return
        print(f"sector {'AB'[cur]}, sequence {images[cur].seq}")
        for key, value in sorted(images[cur].records.items()):
            print("  " + describe(key, value))
        return

    if args.cmd == "set-address":
        if not 1 <= args.address <= 239:
            sys.exit("address must be 1..239")
        key, value = KEY_ADDRESS, bytes([args.address])
    else:
        value = args.name.encode()
        if not 1 <= len(value) <= 32 or not args.name.isprintable():
            sys.exit("name must be 1..32 printable characters")
        key = KEY_NAME

    img = Image(images[cur].seq + 1 if cur is not None else 1, images[cur].records if cur is not None else {})
    img.records[key] = value
    target = 0 if cur is None else 1 - cur
    ocd.write_sector(base + target * SECTOR_SIZE, img.encode())
    print(f"ok {describe(key, value)} (sector {'AB'[target]}, sequence {img.seq})")


if __name__ == "__main__":
    main()
