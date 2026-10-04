# 0001: PC ↔ CAN link

- **Status:** Accepted
- **Date:** 2026-10-04
- **Issue:** #12

## Context

A PC needs access to the CAN FD bus to flash CIMs, send commands (ping, reboot, configuration) and log traffic. PCs have no CAN port, so a USB device has to sit in between.

Constraints:

- **A CIM on its own must be enough.** No bought adapter (PCAN, Kvaser, …) may be required. If the CIM speaks the same protocol as existing adapters, those adapters may work too, but only as a bonus.
- **CAN FD** with 64-byte frames, typically 500 kbit/s nominal and 2 Mbit/s data.
- Team members use **Linux and Windows**. PC tools are written in **Python**.
- The project is BSD-3. Dependencies must be compatible with that.

Until now, a CIM ran a custom "bridge" firmware. It appeared as a USB serial port, accepted hex-text shell commands (`#tcan writex 0x101 <hex>`) and returned raw 64-byte frames. Only CIM's own tools understood it, and it mixed log output into the binary stream.

## Options

| Option | CAN FD on Linux | CAN FD on Windows | Tool support | Firmware effort |
|---|---|---|---|---|
| **gs_usb on the CIM** | Yes: native `can0` through the kernel `gs_usb` driver (FD since Linux 5.18) | Via libusb/WinUSB from Python, through third-party libraries | SocketCAN, can-utils, Wireshark, python-can, cantools, SavvyCAN | Medium (~600–1000 lines) |
| slcan (Lawicel, text over USB serial) | No: the kernel `slcan` driver has no FD support | COM port; FD only through non-standard extensions | python-can (FD partially) | Low |
| Custom USB serial protocol (cleaned up) | COM port | COM port | None; everything self-made, including a python-can plugin | Low (firmware), medium (PC) |
| Bought adapter (PCAN-USB FD, Kvaser) | Yes | Yes | Everything | None, but **violates the constraint** |

### gs_usb in short

gs_usb (also called the *candleLight* protocol) is an open, binary USB protocol used by many open-source USB-CAN adapters. Linux has a built-in driver. A device that speaks gs_usb appears as a CAN network interface:

```sh
sudo ip link set can0 up type can bitrate 500000 dbitrate 2000000 fd on
candump can0
```

On Windows there is no OS driver. With WCID/Microsoft OS descriptors in the firmware, Windows binds the generic WinUSB driver automatically, with no driver installation. Python then talks to the device through libusb.

## Decision

1. **The CIM bridge firmware implements gs_usb with CAN FD**, including WCID descriptors for driverless use on Windows. The Linux kernel driver serves as the specification. [candleLight_fw](https://github.com/candle-usb/candleLight_fw) (MIT) and its FD fork are protocol references only; they are STM32 code.
2. **All PC tools (flasher, commands, logger) use only [python-can](https://python-can.readthedocs.io/)** (`can.Bus`) with FD frames. Interface and channel come from configuration.
   - On Linux, they use `interface="socketcan"` with the CIM as `can0`.
   - On Windows, they use a gs_usb FD backend over libusb. python-can's built-in `gs_usb` interface supports classic CAN only, so we evaluate [gsusb-canfd](https://github.com/sorrowfeng/gsusb-canfd) (MIT) first, and otherwise write a small python-can plugin of our own.
3. **slcan is not supported. The old custom bridge protocol is dropped** once gs_usb works.

## Consequences

- With one CIM on USB, every CIM on the bus can be reached with standard tools. On Linux, users get Wireshark, candump and DBC decoding with cantools at no extra cost.
- As a bonus, existing gs_usb adapters (e.g. candleLight FD) and any other python-can interface work with our tools.
- The Windows FD path depends on a third-party library or our own plugin. It is less mainstream than Linux and must be tested on Windows explicitly.
- More firmware effort than a text protocol: USB vendor class, around 10 control requests, TX echo, timestamps, and 80-byte FD frames split over two USB packets.

## Open points

- **USB VID/PID:** the Linux `gs_usb` driver binds only to known IDs. We need to decide which ID the CIM uses: an existing gs_usb ID (if allowed), an ID from [pid.codes](https://pid.codes/) plus a kernel patch, or binding with `new_id` via sysfs as a stopgap. This is the first thing to clarify before implementation.
- **SPI throughput:** at 2 MHz SPI the CIM cannot keep up with a fully loaded 2 Mbit/s FD bus. Sniffing needs roughly 10–15 MHz SPI and batched FIFO reads. Thesis issue [bluuas/cim-mt#79](https://github.com/bluuas/cim-mt/issues/79) showed SPI failures at ≥ 14 MHz only together with `copy_to_ram`, so this must be retested.
- **Windows backend:** choose between gsusb-canfd and a plugin of our own after a hardware test.

## References

- Linux gs_usb CAN FD support: [Pengutronix blog: candleLight FD](https://pengutronix.de/en/blog/2023-08-17-candlelight-fd-open-hardware-usb-to-can-fd-interface.html)
- [candleLight_fw](https://github.com/candle-usb/candleLight_fw), [candleLightFD fork](https://github.com/linux-automation/candleLightFD)
- [python-can gs_usb interface](https://python-can.readthedocs.io/en/stable/interfaces/gs_usb.html), [gsusb-canfd](https://github.com/sorrowfeng/gsusb-canfd)
