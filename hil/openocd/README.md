# OpenOCD configs for the HIL slots

One config per slot of the HIL HAT. OpenOCD on the Pi drives SWD directly from the Pi's GPIOs (`linuxgpiod` adapter of the Raspberry Pi OpenOCD build, see `hil/setup-pi.sh`), no debug probe needed.

| Config | Slot | SWCLK | SWDIO | BOOTSEL (unused) |
|---|---|---|---|---|
| `slot-a.cfg` | TARGET A (U1) | GPIO26 | GPIO16 | GPIO21 |
| `slot-b.cfg` | TARGET B (U2) | GPIO6 | GPIO13 | GPIO19 |
| `slot-c.cfg` | TARGET C (U3) | GPIO27 | GPIO23 | GPIO22 |

## Use

On the Pi, from the repository root:

```sh
# flash, verify, reset (about 5 s)
openocd -f hil/openocd/slot-b.cfg -c "program app.elf verify reset exit"

# halt and read the PC, then let it run again
openocd -f hil/openocd/slot-b.cfg -c init -c halt -c "reg pc" -c "rp2040.core0 resume" -c shutdown

# debug with GDB from another machine: target extended-remote <pi-ip>:3333
openocd -f hil/openocd/slot-b.cfg -c "bindto 0.0.0.0"
```

`linuxgpiod` does not support `adapter speed`; the bit-bang rate is fixed by the Pi.

Every OpenOCD instance opens the GDB, telnet and Tcl ports (3333, 4444, 6666). To run several slots at the same time, disable them or give each instance its own ports:

```sh
openocd -f hil/openocd/slot-b.cfg -c "gdb_port disabled; telnet_port disabled; tcl_port disabled" -c "program app.elf verify reset exit"
```

## Reliability check

`flash-cycles.sh` flashes an ELF repeatedly and counts failures. On the bench (proto v7, `tcan_probe`), slots B and C passed 100 of 100 cycles each, run in parallel, 5–6 s per cycle.

```sh
hil/openocd/flash-cycles.sh b build/cim_proto_v7-debug/examples/tcan_probe/tcan_probe.elf 100
```
