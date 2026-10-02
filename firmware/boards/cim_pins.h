/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------
// Pinout shared by all CIM boards (identical on cim-prototype-v7 and cim-pcb-v0).
// Source: netlists of AMZ-Racing/cim-hardware.

#ifndef _BOARDS_CIM_PINS_H
#define _BOARDS_CIM_PINS_H

// --- RGB LED (common anode to +3V3, so active-low) ---
#define CIM_LED_R_PIN 10
#define CIM_LED_G_PIN 11
#define CIM_LED_B_PIN 12
#define CIM_LED_ACTIVE_LOW 1

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN CIM_LED_G_PIN
#endif
#ifndef PICO_DEFAULT_LED_PIN_INVERTED
#define PICO_DEFAULT_LED_PIN_INVERTED 1
#endif

// --- TCAN4551 CAN FD controller (SPI0) ---
#define CIM_TCAN_SPI       0
#define CIM_TCAN_SCLK_PIN  6
#define CIM_TCAN_SDI_PIN   3  // MCU TX -> TCAN SDI
#define CIM_TCAN_SDO_PIN   4  // TCAN SDO -> MCU RX
#define CIM_TCAN_nCS_PIN   5
#define CIM_TCAN_nINT_PIN  2
#define CIM_TCAN_RST_PIN   9
#define CIM_TCAN_nWKRQ_PIN 8
#define CIM_TCAN_GPO1_PIN  7
#define CIM_TCAN_GPO2_PIN  1

// GPIO0/1 are not free for a UART, so there is no PICO_DEFAULT_UART: use USB CDC for stdio.

#endif
