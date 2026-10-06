#!/usr/bin/env python3
"""Show the HIL bench state on the 0.91" OLED on HAT J4 (SSD1306, 128x32).

Line 1: hostname and IP
Line 2: per slot: on (powered), off (empty or unpowered), swd (OpenOCD active)
Line 3: first line of /run/cim-hil/status, written by the HIL runner

The slot pins are read with `pinctrl` (a register read), never as a GPIO line
request, so the display never blocks OpenOCD.
"""
import argparse
import re
import signal
import socket
import subprocess
import sys
import time
from pathlib import Path

# slot: (SWCLK, BOOTSEL), Pi BCM GPIOs, see docs/hil/README.md
SLOTS = {"A": (26, 21), "B": (6, 19), "C": (27, 22)}
STATUS_FILE = Path("/run/cim-hil/status")

PINCTRL_LINE = re.compile(r"^\s*(\d+):\s+(\w+)\s.*\|\s+(hi|lo)\b")


def parse_pinctrl(text):
    """Map GPIO number to (function, level) from `pinctrl get` output."""
    pins = {}
    for line in text.splitlines():
        m = PINCTRL_LINE.match(line)
        if m:
            pins[int(m.group(1))] = (m.group(2), m.group(3))
    return pins


def slot_state(pins, swclk, bootsel):
    """OpenOCD drives SWCLK as output while active. BOOTSEL (QSPI_SS) has a
    pull-up on the CIM and a pull-down on the Pi, so it reads high only when
    the CIM is powered. SWDIO is no use for this: OpenOCD leaves it floating."""
    if pins.get(swclk, ("", ""))[0] == "op":
        return "swd"
    if pins.get(bootsel, ("", ""))[1] == "hi":
        return "on"
    return "off"


def read_slots():
    gpios = ",".join(str(g) for pair in SLOTS.values() for g in pair)
    out = subprocess.run(["pinctrl", "get", gpios], capture_output=True, text=True).stdout
    pins = parse_pinctrl(out)
    return {name: slot_state(pins, *pair) for name, pair in SLOTS.items()}


def local_ip():
    """IP of the default route; connecting a UDP socket sends nothing."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("192.0.2.1", 9))
            return s.getsockname()[0]
    except OSError:
        return "no network"


def read_status(path):
    try:
        return path.read_text().splitlines()[0].strip()
    except (OSError, IndexError):
        return ""


def lines(status_file):
    slots = "  ".join(f"{name} {state}" for name, state in read_slots().items())
    return [f"{socket.gethostname()} {local_ip()}", slots, read_status(status_file)]


def open_display(address, rotate):
    from luma.core.interface.serial import i2c
    from luma.oled.device import ssd1306

    return ssd1306(i2c(port=1, address=address), width=128, height=32, rotate=rotate)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--address", type=lambda s: int(s, 0), default=0x3C, help="I2C address (default: 0x3C)")
    p.add_argument("--rotate", type=int, choices=range(4), default=0, help="0..3, in steps of 90 degrees")
    p.add_argument("--interval", type=float, default=1.0, help="update interval in s (default: 1)")
    p.add_argument("--status-file", type=Path, default=STATUS_FILE, help=f"default: {STATUS_FILE}")
    p.add_argument("--print", action="store_true", help="print the lines instead of using the display")
    args = p.parse_args()

    if args.print:
        print("\n".join(lines(args.status_file)))
        return

    from luma.core.render import canvas

    # exit cleanly on systemctl stop, so luma clears the display instead of
    # leaving a stale image
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))
    device = None
    while True:
        try:
            if device is None:
                device = open_display(args.address, args.rotate)
            with canvas(device) as draw:
                for row, text in enumerate(lines(args.status_file)):
                    draw.text((0, row * 11), text, fill="white")
        except OSError:
            # display missing or I2C error: retry, the service keeps running
            device = None
            time.sleep(5)
            continue
        time.sleep(args.interval)


if __name__ == "__main__":
    main()
