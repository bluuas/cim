"""What is in each slot of the HIL bench? Flashes examples/tcan_probe into every
slot of bench.toml and prints SWD, board UID, SPI speed, TCAN device ID and VSUP.
Exits 1 if a slot differs from bench.toml (present, uid).

    cd hil/runner && python3 bench_check.py [--bench ../bench.toml] [--firmware-dir DIR]
"""
import argparse
import sys
import time
import tomllib
from pathlib import Path

from slot import OpenOcdError, Slot, symbol
from test_tcan_probe import FIELDS, PROBE

DEVICE_ID0_TCAN = 0x4E414354
DIVIDER_DEFECT_MAX_BAUD = 1_900_000  # 1.95 MHz on a good chip, 1.79 MHz with the defect of #41


def probe(slot, elf):
    slot.flash(elf)
    slot.wait_var(elf, "probe", PROBE, lambda v: v[1] != 0, timeout=8)
    time.sleep(0.3)
    values = dict(zip(FIELDS, slot.read_var(elf, "probe", PROBE)))
    # the UID follows the struct of test_tcan_probe: 8 x uint32_t, 4 x bool
    address, _ = symbol(elf, "probe")
    uid = slot.read(address + 36, 8).hex().upper()
    return values, uid


def main():
    here = Path(__file__).resolve().parent
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--bench", type=Path, default=here.parent / "bench.toml")
    p.add_argument("--firmware-dir", type=Path, default=here.parents[1] / "build" / "cim_proto_v7-debug")
    args = p.parse_args()

    slots = tomllib.loads(args.bench.read_text())["slots"]
    elf = args.firmware_dir / "examples" / "tcan_probe" / "tcan_probe.elf"
    status = 0
    for name, info in sorted(slots.items()):
        slot = Slot(name, args.bench.parent / info["openocd"])
        try:
            values, uid = probe(slot, elf)
        except (OpenOcdError, KeyError, OSError) as e:
            line = f"slot {name}: no SWD ({str(e).strip().splitlines()[-1][:60]})"
            problem = info.get("present", False)
        else:
            divider = "defective divider" if values["baudrate"] < DIVIDER_DEFECT_MAX_BAUD else "divider ok"
            tcan = "TCAN ok" if values["id0"] == DEVICE_ID0_TCAN else "TCAN missing"
            vsup = "VSUP ok" if values["vsup_ok"] else "no VSUP"
            line = f"slot {name}: uid {uid}, SPI {values['baudrate'] / 1e6:.2f} MHz ({divider}), {tcan}, {vsup}"
            problem = not info.get("present", False) or ("uid" in info and info["uid"] != uid)
        print(line + ("   <-- differs from bench.toml" if problem else ""))
        status |= bool(problem)
    return status


if __name__ == "__main__":
    sys.exit(main())
