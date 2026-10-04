# cim-config: configuration over SWD

Reads and changes the configuration of a CIM (node address, name) over SWD with a debug probe and OpenOCD. This is the fallback for commissioning when a board has no firmware with USB commissioning. The normal way is over USB; see [firmware/commission](../../firmware/commission/README.md).

It implements the same image format as the firmware ([firmware/config](../../firmware/config/README.md)) and writes the new image to the other configuration sector, so a failed write keeps the old configuration.

## Requirements

- Python 3.8 or newer (standard library only)
- OpenOCD with RP2040 support, and a CMSIS-DAP debug probe (e.g. Raspberry Pi Debug Probe)

## Usage

```sh
./cim_config_swd.py show
./cim_config_swd.py set-address 5
./cim_config_swd.py set-name pedalbox
```

The firmware reads the configuration on every access, so running firmware sees the change immediately.

| Option | Default | |
|---|---|---|
| `--flash-size` | `0x200000` (2 MB) | |
| `--interface` | `cmsis-dap` | OpenOCD interface config |
| `--target` | `rp2040` | OpenOCD target config |
| `--speed` | `5000` | SWD clock in kHz |

## Tests

```sh
python3 -m pytest tools/cim-config
```

The tests check parsing and encoding against an image written by the firmware on hardware.
