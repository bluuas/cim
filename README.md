# CIM: CAN FD Interface Module

> or: _CIM Is Modular_

CIM is a small, modular interface between a CAN FD bus and sensors and actuators. It was originally developed for Formula Student racecars. It is built around a Raspberry Pi RP2040/RP2350 microcontroller and a TI TCAN4551 CAN FD controller. Peripherals connect through a 20-pin mezzanine connector.

> **Status:** being restructured into an open-source project. The CAN FD bootloader ([#14](https://github.com/bluuas/cim/issues/14), [#15](https://github.com/bluuas/cim/issues/15)) and the gs_usb adapter firmware ([ADR 0001](docs/adr/0001-pc-can-link.md)) are not here yet. Hardware files are in [AMZ-Racing/cim-hardware](https://github.com/AMZ-Racing/cim-hardware).

## Supported boards

| Board          | MCU                          | `PICO_BOARD`   |
|----------------|------------------------------|----------------|
| CIM v0         | RP2354A (2 MB stacked flash) | `cim_v0`       |
| CIM prototype v7 | RP2040 + W25Q16JV (2 MB)   | `cim_proto_v7` |

The pinout is the same on both boards, see [docs/hardware/pinout.md](docs/hardware/pinout.md) and [firmware/boards/cim_pins.h](firmware/boards/cim_pins.h).

## Building

```sh
git clone --recurse-submodules https://github.com/bluuas/cim.git
cd cim/firmware
cmake --preset cim_v0-debug          # or cim_proto_v7-debug, *-release
cmake --build --preset cim_v0-debug
```

Output: `build/<preset>/examples/blink/blink.uf2`. Toolchain, devcontainer, flashing and first run: [Getting started](docs/getting-started.md).

## Applications

Applications live in [`firmware/apps/<name>`](firmware/apps/README.md) and share one structure; [`firmware/apps/example`](firmware/apps/example) is the template. A separate repository can use CIM as a git submodule and add its own applications the same way, see [firmware/apps/README.md](firmware/apps/README.md#using-cim-as-a-submodule).

## Documentation

The documentation site is at [bluuas.github.io/cim](https://bluuas.github.io/cim/). It is built with mkdocs from the Markdown files in this repository: the module READMEs, the ADRs in [docs/adr](docs/adr/README.md), the [protocol](docs/protocol/cim-protocol.md) and the [HIL test bench](docs/hil/README.md). To preview it locally:

```sh
pip install -r docs/requirements.txt
mkdocs serve
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the workflow (issues, pull requests, CI) and the naming conventions for branches and commits.

## Acknowledgements

CIM started as the master's thesis *CAN FD Interface Module* by Lukas Betschart at the Lucerne University of Applied Sciences and Arts (HSLU), supervised by Prof. Erich Styger, in 2024. The first firmware was built on his [McuLib](https://github.com/ErichStyger/McuOnEclipseLibrary). The boards were designed for, built with and raced by [AMZ Racing](https://www.amzracing.ch/), whose members wrote the first applications and found the bugs in the car. This repository starts with a fresh history; the original work lives on in [bluuas/cim-mt](https://github.com/bluuas/cim-mt), [AMZ-Racing/cim](https://github.com/AMZ-Racing/cim) and [AMZ-Racing/cim-hardware](https://github.com/AMZ-Racing/cim-hardware).

## License

BSD 3-Clause, see [LICENSE](LICENSE).
