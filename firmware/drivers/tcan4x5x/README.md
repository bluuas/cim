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
tcan_err_t err = tcan_init(&cfg);          /* TCAN_ERR_NO_VSUP without 12 V on VSUP */

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
