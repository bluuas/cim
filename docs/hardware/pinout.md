# Pinout

The pinout is identical on CIM prototype v7 and CIM v0. The firmware's single source is [firmware/boards/cim_pins.h](../../firmware/boards/cim_pins.h); this page is taken from the netlists in [AMZ-Racing/cim-hardware](https://github.com/AMZ-Racing/cim-hardware).

## Mezzanine connector J1

Amphenol Bergstak 10132798-021100LF, 20 pins, 0.5 mm pitch, on the bottom side. The shield pins are on GND.

| Pin | Signal | Direction | Notes |
|---|---|---|---|
| 1 | `+BATT` | in | supply input, regulator rated to 36 V; diode-ORed with USB VBUS |
| 2 | `-BATT` | in | supply return |
| 3 | `GND` | | |
| 4 | `+3V3` | out | 3.3 V from the on-board regulator for the carrier |
| 5 | `BOOTSEL` | in | RP2040 `QSPI_SS`; pull low during reset to enter the USB bootloader |
| 6 | `USB_DP` | | USB data, in parallel to the USB-C on the breakaway part |
| 7 | `USB_DM` | | |
| 8 | `SWDIO` | | debug, in parallel to the SWD pads |
| 9 | `SWCLK` | | |
| 10 | `GND` | | |
| 11 | `CANH` | | CAN FD bus; termination belongs on the bus, not on the CIM |
| 12 | `CANL` | | |
| 13 | `GPIO22` | | application I/O |
| 14 | `GPIO23` | | |
| 15 | `GPIO24` | | |
| 16 | `GPIO25` | | |
| 17 | `GPIO26` | | application I/O, `ADC0` |
| 18 | `GPIO27` | | `ADC1` |
| 19 | `GPIO28` | | `ADC2` |
| 20 | `GPIO29` | | `ADC3` |

The eight GPIOs can carry any RP2040 function: GPIO, PWM, I2C, SPI, UART or PIO. Which pairs form an I2C or UART instance follows the GPIO function table in the MCU datasheet. If an application needs more than eight pins, the thesis suggests an I/O expander (TCA9539) on the carrier.

## GPIOs used on the board

| GPIO | Use | Define in `cim_pins.h` |
|---|---|---|
| 1 | TCAN4551 `GPO2` | `CIM_TCAN_GPO2_PIN` |
| 2 | TCAN4551 `nINT`, interrupt to the MCU | `CIM_TCAN_nINT_PIN` |
| 3 | SPI0 TX, to TCAN `SDI` | `CIM_TCAN_SDI_PIN` |
| 4 | SPI0 RX, from TCAN `SDO` | `CIM_TCAN_SDO_PIN` |
| 5 | TCAN4551 `nCS` | `CIM_TCAN_nCS_PIN` |
| 6 | SPI0 SCLK | `CIM_TCAN_SCLK_PIN` |
| 7 | TCAN4551 `GPO1` | `CIM_TCAN_GPO1_PIN` |
| 8 | TCAN4551 `nWKRQ` | `CIM_TCAN_nWKRQ_PIN` |
| 9 | TCAN4551 `RST` | `CIM_TCAN_RST_PIN` |
| 10, 11, 12 | RGB LED red, green, blue; common anode, so active-low | `CIM_LED_*_PIN`, `CIM_LED_ACTIVE_LOW` |
| 22 to 29 | mezzanine connector | |
| 0, 13 to 21 | not connected | |

GPIO 0 is not connected and GPIO 1 belongs to the TCAN, so there is no default UART; firmware output goes over USB CDC and RTT, see [Logging](../../firmware/log/README.md).

## LED colours in the examples

The firmware uses the LED consistently: blinking red means not commissioned, green means running, yellow means a missing supply on the CAN controller, red means failure. Each example's `main.c` states its own mapping.
