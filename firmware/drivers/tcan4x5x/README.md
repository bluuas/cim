# TCAN4x5x driver

Low-level driver for the TI TCAN4550/TCAN4551 CAN FD controller with integrated transceiver. There is no RTOS dependency.

| Directory | Content | Licence |
|---|---|---|
| `ti/` | TI's *TCAN455x Driver Library Demo* **1.2.2** (2020-08-06, [SLLC469](https://www.ti.com/tool/download/SLLC469)): `TCAN4550.c/.h`, `TCAN4x5x_Reg.h`, `TCAN4x5x_Data_Structs.h` | BSD-3-Clause, © 2019 Texas Instruments |
| `port/` | CIM implementation of TI's SPI abstraction (`TCAN4x5x_SPI.h/.c`) on the CIM HAL | BSD-3-Clause, © CIM contributors, derived from TI's interface |
| `include/tcan.h`, `src/tcan.c` | Thin `tcan_*` API: init, send, receive, filters, status | BSD-3-Clause, © CIM contributors |

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
- **Wrong mask for the device interrupt enable register:** `REG_BITS_DEVICE_IE_MASK` (`0x7F69D700`) treats bits 24..30 of register 0x0830 as writable. On the TCAN4550 they always read 1 (measured: writing 0 reads back `0xFF9628FF`), so `TCAN4x5x_Device_ConfigureInterruptEnable()` returns `false` for any value with one of them cleared. `tcan_init()` writes the register itself and checks only the writable bits (`0x0069D700`). Found on the HIL bench, #36.

## Usage

```c
#include "tcan.h"

static const tcan_filter_t filters[] = {
    {.id = 0x100, .mask = 0x700},           /* standard IDs 0x100..0x1FF */
    {.id = 0x18FF0000, .mask = 0x1FFF0000, .ext = true},
};

tcan_config_t cfg = TCAN_CONFIG_DEFAULT;  /* CIM pins, 40 MHz, 500k / 2M, TX queue mode */
cfg.filters = filters;                     /* omit to accept all frames */
cfg.num_filters = 2;
tcan_err_t err = tcan_init(&cfg);          /* TCAN_ERR_NO_VSUP if VSUP is undervoltage */

for (;;) {
    tcan_poll();                           /* cheap when nothing is pending */
    tcan_msg_t rx;
    while (tcan_receive(&rx)) { /* ... */ }

    tcan_msg_t tx = {.id = 0x123, .fd = true, .brs = true, .len = 16};
    tcan_send(&tx);                        /* TCAN_ERR_BUSY if all TX buffers are pending */
}
```

- **Bit timing:** computed from the bit rates. Nominal sample point 80 %, data 75–80 %. Bit rates that do not divide the 40 MHz clock exactly are rejected.
- **TX mode:** `TCAN_TX_QUEUE` (TI's default) sends pending frames by ID priority. `TCAN_TX_FIFO` keeps the send order, e.g. for multi-frame protocols.
- **Polling:** the nINT interrupt only sets a flag. All SPI traffic happens in `tcan_poll()`, `tcan_send()` and `tcan_get_status()`, so call them from one context.
- **TI functions:** anything not covered by the API can be done with TI's functions (`TCAN4550.h`) after `tcan_init()`.

## SPI clock

`TCAN_CONFIG_DEFAULT` requests 2 MHz. Limits and measurements (#102):

- **Datasheet:** TCAN4551-Q1 (SLLSEZ4A), SPI switching characteristics: SCK up to 18 MHz (period 56 ns, high and low 18 ns each), chip select disable time 125 ns, data out valid 20 ns at 3.135 V to 5.25 V VIO. The table is marked "specified by design". The 2 KB message RAM is read in bursts of up to 255 words (one length byte, section 8.5).
- **RP2040:** the SDK derives the SPI clock from the 125 MHz peripheral clock by integer dividers, so the clock actually set is lower than requested: 18 and 20 MHz both give 15.62 MHz. The board with the divider defect of #41 computes other values (2 MHz gives 1.79 MHz, 8 MHz gives 4.19 MHz).
- **Method:** `examples/tcan_spi_bench` writes a pattern into the message RAM and reads it back 5000 times (10.24 MB) at each clock, then writes and reads back 1000 times at the clock under test, and times a 72-byte read (one 64-byte FD receive element). `hil/runner/spi_bench.py` runs it on a slot.

| Requested | Actual (good / defective board) | 72-byte read | Elements per second | Errors in 10.24 MB |
|---|---|---|---|---|
| 2 MHz | 1.95 / 1.79 MHz | 371 / 402 µs | 2700 / 2500 | none |
| 5 MHz | 4.81 / 4.19 MHz | 152 / 287 µs | 6600 / 3500 | none |
| 10 MHz | 8.93 / 8.93 MHz | 83 / 84 µs | 12 000 / 12 000 | none |
| 12 MHz | 10.42 / 10.49 MHz | 71 / 78 µs | 14 000 / 12 800 | none |
| 16 MHz | 15.62 / 12.58 MHz | 48 / 53 µs | 20 700 / 18 800 | none |
| 18 MHz | 15.62 / 16.78 MHz | 48 / 49 µs | 20 700 / 20 500 | none |
| 24 MHz (out of spec) | 20.83 / 20.97 MHz | 37 / 41 µs | 27 000 / 24 400 | good board none, defective board 2700 of 5000 reads wrong |

Running from RAM (`copy_to_ram`) gave the same results on both boards, and the TCAN's SPIERR flag stayed 0 in every run. The throughput stays below the wire rate (1.8 MB/s at 21 MHz) because the SDK's SPI read loop handles one byte at a time.
