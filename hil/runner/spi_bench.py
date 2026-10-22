"""SPI clock measurement of the TCAN4551 link (#102): flashes examples/tcan_spi_bench
into a slot, waits until all clocks are measured and prints a table.

    cd hil/runner && python3 spi_bench.py --slot b [--ram] [--firmware-dir DIR]

--ram uses the build that copies the program to RAM at start-up.
"""
import argparse
import tomllib
from pathlib import Path

from slot import Slot

RESULT = ("requested_hz", "actual_hz", "reads", "bad_reads", "bad_bytes", "read_us", "rw_iters", "rw_bad",
          "elem_us_x100", "spierr")
N_CLOCKS = 10
FORMAT = "<3I" + "10I" * N_CLOCKS
READ_BYTES = 2048


def main():
    here = Path(__file__).resolve().parent
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--slot", required=True)
    p.add_argument("--ram", action="store_true")
    p.add_argument("--bench", type=Path, default=here.parent / "bench.toml")
    p.add_argument("--firmware-dir", type=Path, default=here.parents[1] / "build" / "cim_proto_v7-release")
    args = p.parse_args()

    info = tomllib.loads(args.bench.read_text())["slots"][args.slot]
    slot = Slot(args.slot, args.bench.parent / info["openocd"])
    name = "tcan_spi_bench_ram" if args.ram else "tcan_spi_bench"
    elf = args.firmware_dir / "examples" / "tcan_spi_bench" / f"{name}.elf"

    slot.flash(elf)
    values = slot.wait_var(elf, "bench", FORMAT, lambda v: v[0] == 1, timeout=400)
    done, _, id0 = values[:3]
    print(f"{name} in slot {args.slot} (uid {info.get('uid', '?')}): done={done}, TCAN ID 0x{id0:08X}")
    print("request    actual  MB/s read  bad reads  bad bytes  rw bad   72 B read   frames/s  SPIERR")
    for i in range(N_CLOCKS):
        r = dict(zip(RESULT, values[3 + i * 10:3 + (i + 1) * 10]))
        if r["requested_hz"] == 0:
            continue
        mbs = r["reads"] * READ_BYTES / r["read_us"] if r["read_us"] else 0
        elem_us = r["elem_us_x100"] / 100
        print(f"{r['requested_hz'] / 1e6:5.1f} MHz {r['actual_hz'] / 1e6:6.2f}  {mbs:9.2f}  "
              f"{r['bad_reads']:>4}/{r['reads']:<5} {r['bad_bytes']:>9}  {r['rw_bad']:>3}/{r['rw_iters']:<4} "
              f"{elem_us:7.2f} us  {1e6 / elem_us if elem_us else 0:9.0f}  {r['spierr']}")


if __name__ == "__main__":
    main()
