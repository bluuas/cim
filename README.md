# CIM: CAN FD Interface Module

> or: _CIM Is Modular_

CIM is a small, modular interface between a CAN FD bus and sensors and actuators. It was originally developed for Formula Student racecars. It is built around a Raspberry Pi RP2040/RP2350 microcontroller and a TI TCAN4551 CAN FD controller. Peripherals connect through a 20-pin mezzanine connector.

> **Status:** this repository is being restructured into an open-source project. Only the board support and a blink example are here so far. The CAN FD driver, bootloader, flasher and hardware files will follow.

## Supported boards

| Board          | MCU                          | `PICO_BOARD`   |
|----------------|------------------------------|----------------|
| CIM v0         | RP2354A (2 MB stacked flash) | `cim_v0`       |
| CIM prototype v7 | RP2040 + W25Q16JV (2 MB)   | `cim_proto_v7` |

The pinout is the same on both boards; see [firmware/boards/cim_pins.h](firmware/boards/cim_pins.h).

## Building

The easiest way is the devcontainer: open the repo in VS Code and choose *Reopen in Container*. It provides the ARM toolchain, Pico SDK, picotool and openocd.

```sh
cd firmware
cmake --preset cim_v0-debug          # or cim_proto_v7-debug, *-release
cmake --build --preset cim_v0-debug
```

The output is in `build/<preset>/examples/blink/blink.uf2`.

Without the devcontainer you need `arm-none-eabi-gcc`, CMake ≥ 3.13, Ninja and the [Pico SDK](https://github.com/raspberrypi/pico-sdk) ≥ 2.1, with `PICO_SDK_PATH` set.

## Flashing

Hold BOOTSEL while plugging in USB, then drag-and-drop the `.uf2` file, or run:

```sh
picotool load -x build/cim_v0-debug/examples/blink/blink.uf2
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the workflow (issues, pull requests, CI) and the naming conventions for branches and commits.

## License

BSD 3-Clause, see [LICENSE](LICENSE).
