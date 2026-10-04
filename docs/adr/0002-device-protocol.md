# 0002: Device and bootloader protocol

- **Status:** Accepted
- **Date:** 2026-10-04
- **Issue:** #13

## Context

The PC (through a CIM acting as gs_usb adapter, see [ADR 0001](0001-pc-can-link.md)) has to talk to the CIMs on the bus to:

- find out which CIMs are on the bus (with address, UID and firmware version),
- reboot them, including into the bootloader,
- update their firmware (images of ~100–500 KB) through a bare-metal bootloader with a ~90 KB flash budget,
- later: read and write configuration (e.g. calibration values).

Every CIM has a fixed address by design (e.g. pedalbox = 5). It is stored in the board's flash configuration, because all boards run the same bootloader, which must know its address too.

Constraints:

- The bus is shared with the car's ECUs. The team DBC uses 106 standard (11-bit) IDs and no extended IDs.
- Up to ~30 CIMs on one bus. CAN FD with 64-byte frames.
- Students on the team must be able to understand, debug and extend the protocol. It has to fit on one or two pages.

The old protocol (*CanShell*) used one shared 11-bit ID (0x101) for requests and responses and a 4-byte header with source and destination address. Its flaws were a shared request/response ID, no address range checks, silent failures (no reply on errors), no protocol version, and segmentation of up to 1 KB through an `end_flag` without sequence numbers.

## Options

| Option | Summary | Why not / why |
|---|---|---|
| ISO-TP + UDS (ISO 15765-2 / ISO 14229) | Automotive standard for diagnostics and flashing. MIT libraries for C (iso14229, isotp-c) and Python (udsoncan, can-isotp). Wireshark decodes it. | Many concepts (sessions, ~30 services, ~40 error codes, flow control, timing parameters) for a simple job. Hard for the team to understand and debug. ~15–20 KB of third-party code in the bootloader, with iso14229 still at 0.y. |
| CANopen + LSS | Object dictionary, SDO transfers, LSS for node ID assignment | CANopenNode has no CAN FD support. Uses large parts of the 11-bit ID space, which conflicts with the team DBC. |
| XCP on CAN | Measurement and calibration protocol | No open CAN slave implementation; meant for calibration, not bootloading |
| OpenBLT, Katapult | Existing open CAN bootloaders | GPLv3 (incompatible with BSD-3). Katapult's UID-based discovery is a good model. |
| **Thesis protocol (CAN Shell), revised** | The protocol from the master thesis, with its known flaws fixed | **Chosen** |

## Decision

The **CIM protocol, version 2**, specified in [docs/protocol/cim-protocol.md](../protocol/cim-protocol.md). Version 1 is the *CAN Shell* protocol from the master thesis. Version 2 keeps its concept and flow (PING, REBOOT, and the bootloader stages INFO, ERASE, WRITE, SEAL, GO derived from usedbytes' serial bootloader) and fixes its flaws:

- **29-bit IDs** with frame type, destination and source address in the ID. Separate IDs for requests, responses and data. Lowest priority, and no 11-bit IDs from the DBC are used.
- **No segmentation:** one CAN FD frame is one command. Bulk data frames carry their own offset.
- **Every request gets a response** with a status code from a short list. No silent failures.
- **Protocol version** in the PING response.
- **Fixed addresses, set during commissioning over USB or SWD**, not over CAN. A CIM without a configured address stays silent on the bus. A broadcast PING lists all CIMs with their address, UID (the RP2040's 64-bit flash unique ID) and firmware version.
- **Flashing in 4 KB blocks** (one flash sector): data frames are sent as a window, then the CIM confirms the whole block with CRC32. One round trip per block.

The bootloader and the application implement the same basic commands (PING, REBOOT), so the PC can reach any CIM regardless of what it is running.

## Consequences

- The changes from version 1 are listed in the specification, so the thesis stays usable as background.
- The name *CAN Shell* is dropped because the protocol has no shell. The protocol fits on two pages in our repo, and the code is our own: an estimated 2–4 KB in the bootloader, and ~300 lines of Python on python-can.
- We own the specification and the edge cases, and we must test them ourselves (host tests with a simulated device).
- No native Wireshark decoding. An optional Lua dissector can be added later.
- Diagnostics such as error codes (DTCs) are not covered. If needed, UDS can be added **to the application** later without changing the bootloader.
- Because the protocol only uses 29-bit IDs, it can share the bus with the old CanShell protocol (11-bit ID 0x101).

## Open points

- **Compatibility with AMZ's CIMs in the field** (old CanShell bootloader): to be decided. Options are a one-time SWD reflash during migration, or keeping the old flasher in AMZ's repository until all boards are migrated.
- **Commissioning tool:** how the address is written over USB or SWD (part of the configuration work in milestone M3).
- **Configuration commands** (e.g. calibration values) are reserved in the command space but specified later (milestone M3).
