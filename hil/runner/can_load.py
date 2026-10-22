"""CAN load test (#102): examples/can_load sends frames back to back from one slot
and counts them in another. The receiver's SPI clock is the parameter, the
sender uses a fixed fast clock so it is never the bottleneck.

    cd hil/runner && python3 can_load.py --tx b --rx c --spi 10e6 12e6 18e6 --data 8e6

Needs +BATT (12 V to 24 V, JP1 open) on both slots, see docs/hil/README.md.
"""
import argparse
import time
import tomllib
from pathlib import Path

from slot import Slot, symbol

# examples/can_load/main.c, struct load
FIELDS = ("go", "role", "spi_hz", "nominal_bps", "data_bps", "frames", "len", "init_err", "sent", "send_busy",
          "send_err", "received", "gaps", "bad", "rx_lost", "rx_overflows", "tx_errors", "rx_errors", "bus_off",
          "last_error", "last_data_error", "elapsed_us", "done")
FORMAT = "<7I" + "i" + "15I"
INPUTS = ("go", "role", "spi_hz", "nominal_bps", "data_bps", "frames", "len")


def read(slot, elf):
    return dict(zip(FIELDS, slot.read_var(elf, "load", FORMAT)))


def start(slot, elf, **inputs):
    """Write the inputs into RAM (go last) while the firmware waits for go."""
    address, _ = symbol(elf, "load")
    offset = {name: 4 * i for i, name in enumerate(FIELDS)}
    commands = ["init"]
    for name in INPUTS:
        if name != "go" and name in inputs:
            commands.append(f"mww 0x{address + offset[name]:08X} 0x{int(inputs[name]):08X}")
    commands += [f"mww 0x{address:08X} 1", "shutdown"]
    slot.openocd(*commands)


def main():
    here = Path(__file__).resolve().parent
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--tx", required=True)
    p.add_argument("--rx", required=True)
    p.add_argument("--spi", type=float, nargs="+", default=[10e6, 12e6, 18e6], help="receiver SPI clocks (Hz)")
    p.add_argument("--tx-spi", type=float, default=18e6)
    p.add_argument("--nominal", type=float, default=500e3)
    p.add_argument("--data", type=float, default=8e6)
    p.add_argument("--len", type=int, default=64)
    p.add_argument("--frames", type=int, default=100000)
    p.add_argument("--bench", type=Path, default=here.parent / "bench.toml")
    p.add_argument("--firmware-dir", type=Path, default=here.parents[1] / "build" / "cim_proto_v7-release")
    args = p.parse_args()

    slots = tomllib.loads(args.bench.read_text())["slots"]
    tx = Slot(args.tx, args.bench.parent / slots[args.tx]["openocd"])
    rx = Slot(args.rx, args.bench.parent / slots[args.rx]["openocd"])
    elf = args.firmware_dir / "examples" / "can_load" / "can_load.elf"

    print(f"{args.len} byte frames, {args.nominal / 1e3:.0f}k / {args.data / 1e6:.0f}M, {args.frames} frames, "
          f"sender {args.tx} at {args.tx_spi / 1e6:.0f} MHz requested, receiver {args.rx}")
    print("rx SPI     sent  received    gaps  bad  rx_lost  overflow  busy  tx_err rx_err bus_off  frames/s  note")
    for spi in args.spi:
        rx.flash(elf)
        tx.flash(elf)
        start(rx, elf, role=0, spi_hz=spi, nominal_bps=args.nominal, data_bps=args.data)
        time.sleep(1.0)
        rxs = read(rx, elf)
        if rxs["init_err"] != 0:
            print(f"{spi / 1e6:4.0f} MHz  receiver init failed ({rxs['init_err']})")
            continue
        start(tx, elf, role=1, spi_hz=args.tx_spi, nominal_bps=args.nominal, data_bps=args.data,
              frames=args.frames, len=args.len)
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline:
            txs = read(tx, elf)
            if txs["done"] or txs["init_err"] != 0:
                break
            time.sleep(0.5)
        time.sleep(0.5)
        txs, rxs = read(tx, elf), read(rx, elf)
        rate = txs["sent"] / (txs["elapsed_us"] / 1e6) if txs["elapsed_us"] else 0
        note = "tx init failed" if txs["init_err"] else ("tx timeout" if not txs["done"] else "")
        print(f"{spi / 1e6:4.0f} MHz {txs['sent']:8} {rxs['received']:9} {rxs['gaps']:7} {rxs['bad']:4} "
              f"{rxs['rx_lost']:8} {rxs['rx_overflows']:9} {txs['send_busy']:5} {txs['tx_errors']:6} "
              f"{rxs['rx_errors']:6} {txs['bus_off']:7} {rate:9.0f}  {note}")


if __name__ == "__main__":
    main()
