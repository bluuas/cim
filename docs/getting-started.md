# Getting started

This page takes you from a clone to a CIM that blinks, talks over USB and has a node address. Afterwards, [write your own application](../firmware/apps/README.md).

```mermaid
flowchart LR
    build["Build<br/>cmake preset"] --> flash["Flash<br/>UF2 or SWD"]
    flash --> run["Run<br/>LED + USB serial"]
    run --> commission["Commission<br/>set address"]
    commission --> app["Your application"]
```

## What you need

- A CIM board, either CIM v0 or CIM prototype v7 (see [Hardware overview](hardware/overview.md)). USB-C powers the board and is enough for everything on this page.
- For CAN: a supply on `+BATT` through the mezzanine connector (12 V on the bench, the 24 V LV system in the car). Over USB alone the CAN controller reports undervoltage and stays off.
- Optional: a debug probe (CMSIS-DAP) on the SWD pads, or the [HIL test bench](hil/README.md).

## Get the code

FreeRTOS is a git submodule:

```sh
git clone --recurse-submodules https://github.com/bluuas/cim.git
cd cim
```

In an existing clone, run `git submodule update --init`.

## Build

The easiest way is the devcontainer: open the repository in VS Code and choose *Reopen in Container*. It provides the ARM toolchain, the Pico SDK, picotool and OpenOCD. Without it you need `arm-none-eabi-gcc`, CMake 3.13 or newer, Ninja and the [Pico SDK](https://github.com/raspberrypi/pico-sdk) 2.1 or newer, with `PICO_SDK_PATH` set.

There is one CMake preset per board and build type:

| Preset | Board |
|---|---|
| `cim_v0-debug`, `cim_v0-release` | CIM v0 (RP2354A) |
| `cim_proto_v7-debug`, `cim_proto_v7-release` | CIM prototype v7 (RP2040) |

```sh
cd firmware
cmake --preset cim_proto_v7-debug
cmake --build --preset cim_proto_v7-debug
```

The build directory is `build/<preset>/` at the repository root. Every example and application produces `.elf`, `.uf2` and `.bin`, for instance `build/cim_proto_v7-debug/examples/blink/blink.uf2` and `build/cim_proto_v7-debug/apps/example/example.uf2`.

## Flash

**Over USB (UF2).** Hold the BOOTSEL button on the breakaway part while plugging in USB; the board shows up as a mass-storage device. Drag the `.uf2` onto it, or use picotool:

```sh
picotool load -x build/cim_proto_v7-debug/examples/blink/blink.uf2
```

A board that already runs firmware with commissioning restarts into the USB bootloader with the serial command `reboot bootsel`, so you do not need to reach the button.

**Over SWD.** With a CMSIS-DAP probe on the SWD pads (or the pads of the mezzanine connector):

```sh
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg \
    -c "adapter speed 5000" -c "program build/cim_proto_v7-debug/examples/blink/blink.elf verify reset exit"
```

Use `target/rp2350.cfg` for CIM v0. On the HIL bench the Pi flashes the boards over its GPIOs, see [OpenOCD per slot](../hil/openocd/README.md).

## First run

The examples in `firmware/examples/` are small, bare-metal checks of single modules. Start with `blink`: it cycles the RGB LED through its colours and prints a heartbeat over USB.

Firmware output goes to the USB serial port (CDC) and, in parallel, to an RTT buffer readable over SWD. Open the serial port with any terminal:

```sh
picocom -b 115200 /dev/ttyACM0      # Linux; PuTTY on Windows
```

| Example | Shows |
|---|---|
| `blink` | LED and USB output |
| `log_demo` | the [logging](../firmware/log/README.md) levels, USB and RTT |
| `config_counter` | the [configuration store](../firmware/config/README.md): a boot counter that survives resets |
| `commission` | [commissioning](../firmware/commission/README.md) over USB serial |
| `rtos_demo` | the [FreeRTOS](../firmware/rtos/README.md) integration |
| `tcan_probe` | the [TCAN4x5x driver](../firmware/drivers/tcan4x5x/README.md): reset, device ID, interrupt |

## Give the board an address

Every CIM has a node address and a name, stored in flash. Flash `apps/example` or `examples/commission`, open the serial port and type:

```
set address 5
set name pedalbox
info
```

The LED blinks red until an address is set and shows green afterwards. All commands, and the SWD fallback for boards without firmware, are in [Commissioning](../firmware/commission/README.md).

## Next steps

- [Writing an application](../firmware/apps/README.md): the structure every app shares, and how to use CIM as a submodule in your own repository.
- [CIM protocol](protocol/cim-protocol.md): how a host talks to the boards over CAN.
- [Contributing](../CONTRIBUTING.md): branches, commits, pull requests.
