# TCAN4x5x driver

Low-level driver for the TI TCAN4550/TCAN4551 CAN FD controller with integrated transceiver. There is no RTOS dependency.

| Directory | Content | Licence |
|---|---|---|
| `ti/` | TI's *TCAN455x Driver Library Demo* **1.2.2** (2020-08-06, [SLLC469](https://www.ti.com/tool/download/SLLC469)): `TCAN4550.c/.h`, `TCAN4x5x_Reg.h`, `TCAN4x5x_Data_Structs.h` | BSD-3-Clause, © 2019 Texas Instruments |
| `port/` | CIM implementation of TI's SPI abstraction (`TCAN4x5x_SPI.h/.c`) on the CIM HAL | BSD-3-Clause, © CIM contributors, derived from TI's interface |

## Rules for `ti/`

- The files are **byte-identical** to TI's release, including CRLF line endings, which `.gitattributes` preserves. Do not edit them. Fix or work around problems in our own code, so we can diff against future TI releases.
- TI's `TCAN4x5x_SPI.c/.h` (MSP430 port) and the demo's `main.c` and MSP430 driverlib are not included.

## Configuration

`TCAN4550.h` enables all of TI's options by default, and we keep them:

| Option | Effect |
|---|---|
| `TCAN4x5x_MCAN_CACHE_CONFIGURATION` | Caches MRAM layout registers in RAM. This saves 2 SPI reads per sent or received message. The cache is filled by `TCAN4x5x_MRAM_Configure()`, so configure the MRAM only through that function. |
| `TCAN4x5x_MCAN_VERIFY_CONFIGURATION_WRITES` | Reads back MCAN configuration writes and returns `false` on mismatch |
| `TCAN4x5x_DEVICE_VERIFY_CONFIGURATION_WRITES` | Same for device registers. `TCAN4x5x_Device_SetMode(NORMAL)` fails if the device does not enter Normal mode, e.g. when VSUP is undervoltage (UVSUP). |

## Known issues in TI's code

- **Uninitialised return value for DLC 0:** `TCAN4x5x_MCAN_ReadNextFIFO()` and `TCAN4x5x_MCAN_ReadRXBuffer()` return an uninitialised byte count when a zero-length frame is received. Use `TCAN4x5x_MCAN_DLCtoBytes(header.DLC)` instead of the return value. The compiler warning is silenced for `ti/TCAN4550.c` only. Tracked in #34.

## Usage

```c
TCAN4x5x_SPI_Init(&spi_dev);   /* cim_spi_dev_t with pins from boards/cim_pins.h */
uint32_t id = AHB_READ_32(REG_SPI_DEVICE_ID0);
```

See `examples/tcan_probe`. A higher-level API (init, send, receive, filters) is planned on top of this driver.
