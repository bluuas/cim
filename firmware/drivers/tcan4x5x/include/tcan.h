/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Thin, RTOS-agnostic API for the TCAN4x5x on top of TI's driver library.
 *
 * Usage:
 *   tcan_config_t cfg = TCAN_CONFIG_DEFAULT;
 *   cfg.nominal_bitrate = 500000;
 *   cfg.data_bitrate = 2000000;
 *   if (tcan_init(&cfg) != TCAN_OK) { ... }
 *
 *   for (;;) {
 *       tcan_poll();                   // move received frames into the RX buffer
 *       tcan_msg_t msg;
 *       while (tcan_receive(&msg)) { ... }
 *       tcan_send(&msg);
 *   }
 *
 * The nINT interrupt only sets a flag; all SPI traffic happens in tcan_poll(),
 * tcan_send() and tcan_get_status(), which must not be called concurrently.
 * TI's functions (TCAN4550.h) may be used after tcan_init() for anything not
 * covered here.
 */

#ifndef TCAN_H
#define TCAN_H

#include <stdbool.h>
#include <stdint.h>

#include "cim/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Number of frames buffered in RAM between tcan_poll() and tcan_receive(). */
#ifndef TCAN_RX_BUFFER_SIZE
#define TCAN_RX_BUFFER_SIZE 32
#endif

/** Maximum number of acceptance filters per ID type (standard / extended). */
#define TCAN_MAX_FILTERS 16

typedef enum {
    TCAN_OK = 0,
    TCAN_ERR_PARAM = -1,    /**< invalid argument (bit rate, length, filter count) */
    TCAN_ERR_DEVICE = -2,   /**< no TCAN4x5x answers on SPI (device ID mismatch) */
    TCAN_ERR_CONFIG = -3,   /**< a configuration write could not be verified */
    TCAN_ERR_NO_VSUP = -4,  /**< VSUP undervoltage: the transceiver cannot be enabled */
    TCAN_ERR_MODE = -5,     /**< device did not enter Normal mode */
    TCAN_ERR_BUSY = -6,     /**< all TX buffers are pending */
    TCAN_ERR_BUS_OFF = -7,  /**< controller is bus-off */
} tcan_err_t;

/** Order in which pending TX frames are sent. */
typedef enum {
    TCAN_TX_QUEUE, /**< by CAN ID priority (lowest ID first), TI's default */
    TCAN_TX_FIFO,  /**< in the order tcan_send() was called */
} tcan_tx_mode_t;

/** Acceptance filter: a frame matches if (frame_id & mask) == (id & mask). */
typedef struct {
    uint32_t id;
    uint32_t mask;
    bool ext; /**< 29 bit extended ID */
} tcan_filter_t;

typedef struct {
    cim_spi_dev_t spi;
    uint8_t rst_pin;
    uint8_t nint_pin;
    uint32_t clock_hz;        /**< TCAN oscillator, 40 MHz on CIM boards */
    uint32_t nominal_bitrate; /**< arbitration phase, e.g. 500000 */
    uint32_t data_bitrate;    /**< CAN FD data phase, e.g. 2000000; 0 = classic CAN only */
    tcan_tx_mode_t tx_mode;
    const tcan_filter_t *filters; /**< NULL / 0: accept all frames */
    uint8_t num_filters;
} tcan_config_t;

/** Default configuration for CIM boards (pins from boards/cim_pins.h). */
#define TCAN_CONFIG_DEFAULT                                                                                  \
    {                                                                                                        \
        .spi = {.spi = CIM_TCAN_SPI,                                                                         \
                .sck_pin = CIM_TCAN_SCLK_PIN,                                                                \
                .mosi_pin = CIM_TCAN_SDI_PIN,                                                                \
                .miso_pin = CIM_TCAN_SDO_PIN,                                                                \
                .cs_pin = CIM_TCAN_nCS_PIN,                                                                  \
                .baudrate = 2000000},                                                                        \
        .rst_pin = CIM_TCAN_RST_PIN, .nint_pin = CIM_TCAN_nINT_PIN, .clock_hz = 40000000,                    \
        .nominal_bitrate = 500000, .data_bitrate = 2000000, .tx_mode = TCAN_TX_QUEUE, .filters = NULL,       \
        .num_filters = 0,                                                                                    \
    }

typedef struct {
    uint32_t id;
    bool ext;           /**< 29 bit extended ID */
    bool fd;            /**< CAN FD frame */
    bool brs;           /**< bit rate switch (FD only) */
    uint8_t len;        /**< payload bytes: 0..8, or 12/16/20/24/32/48/64 for FD */
    uint16_t timestamp; /**< RX only: MCAN timestamp counter */
    uint8_t data[64];
} tcan_msg_t;

typedef struct {
    bool bus_off;
    bool error_passive;
    bool error_warning;
    uint8_t tx_errors;      /**< transmit error counter */
    uint8_t rx_errors;      /**< receive error counter */
    bool vsup_ok;           /**< no VSUP undervoltage */
    uint32_t rx_lost;       /**< frames lost because the TCAN RX FIFO was full */
    uint32_t rx_overflows;  /**< frames dropped because the RAM RX buffer was full */
} tcan_status_t;

/** Reset and configure the TCAN4x5x and switch it to Normal mode. */
tcan_err_t tcan_init(const tcan_config_t *config);

/** Queue a frame for transmission. Returns TCAN_ERR_BUSY if all TX buffers are pending. */
tcan_err_t tcan_send(const tcan_msg_t *msg);

/**
 * Handle TCAN interrupts and move received frames into the RX buffer.
 * Cheap if nothing is pending (no SPI traffic). Returns the number of frames received.
 */
unsigned tcan_poll(void);

/** Take the oldest received frame from the RX buffer. Returns false if empty. */
bool tcan_receive(tcan_msg_t *msg);

/** Read error counters and state. */
void tcan_get_status(tcan_status_t *status);

/** Payload length in bytes for a CAN DLC (0..15). */
uint8_t tcan_dlc_to_len(uint8_t dlc);

/** DLC for a payload length; len must be 0..8, 12, 16, 20, 24, 32, 48 or 64. Returns 0xFF if invalid. */
uint8_t tcan_len_to_dlc(uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* TCAN_H */
