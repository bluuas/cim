/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Commissioning over USB serial: set the node address and name of a new board
 * (ADR 0002). Any application can offer it by calling cim_commission_poll()
 * regularly, e.g. in its main loop.
 *
 * Commands (one per line, answers start with "ok" or "error"):
 *
 *   help                     list commands
 *   info                     ok address=5 name=pedalbox uid=E6614103E7452D2F
 *   uid                      ok uid=E6614103E7452D2F
 *   get address              ok address=5            (0: not configured)
 *   set address <1..239>     ok address=5
 *   get name                 ok name=pedalbox
 *   set name <text>          ok name=pedalbox        (max. 32 characters)
 *   reboot                   restart the firmware
 *   reboot bootsel           restart into the USB bootloader (drag & drop UF2)
 */

#ifndef CIM_COMMISSION_H
#define CIM_COMMISSION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CIM_COMMISSION_ADDRESS_MIN 0x01u /**< valid node addresses (CIM protocol v2) */
#define CIM_COMMISSION_ADDRESS_MAX 0xEFu
#define CIM_COMMISSION_NAME_MAX    32u

/**
 * Read available characters from stdin (USB CDC) without blocking and execute
 * complete lines. Answers are written to stdout.
 */
void cim_commission_poll(void);

/**
 * Execute one command line and write the answer (without newline) to out.
 * Separated from the I/O for testing. The reboot commands do not return.
 */
void cim_commission_execute(const char *line, char *out, size_t out_len);

/** Platform hooks, implemented for the Pico SDK in commission_pico.c. */
int cim_commission_getchar(void); /**< next input character, or -1 if none */
void cim_commission_get_uid(uint8_t uid[8]);
void cim_commission_reboot(int bootsel);

#ifdef __cplusplus
}
#endif

#endif /* CIM_COMMISSION_H */
