/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Loads the CAN bus and counts what arrives (#102). One image for both ends;
 * hil/runner/can_load.py writes the parameters into `load` over SWD and sets
 * `go`:
 *   role 0 (receiver): polls the TCAN and counts frames, gaps in the counter
 *       and frames the TCAN or the RAM buffer lost
 *   role 1 (sender): sends `frames` frames with `len` bytes back to back
 * The frame counter is in the first four bytes of the payload.
 */

#include <string.h>

#include "pico/stdlib.h"

#include "cim/hal.h"
#include "tcan.h"

#define TX_ID 0x100u

volatile struct {
    /* written by the runner before go */
    uint32_t go;
    uint32_t role;         /* 0 receive, 1 send */
    uint32_t spi_hz;
    uint32_t nominal_bps;
    uint32_t data_bps;
    uint32_t frames;       /* sender: frames to send */
    uint32_t len;          /* sender: payload bytes */
    /* results */
    int32_t init_err;
    uint32_t sent;
    uint32_t send_busy;    /* tcan_send() found all TX buffers pending */
    uint32_t send_err;
    uint32_t received;
    uint32_t gaps;         /* frames missing in the counter */
    uint32_t bad;          /* frames with another length or payload */
    uint32_t rx_lost;      /* TCAN RX FIFO overflow */
    uint32_t rx_overflows; /* RAM RX buffer overflow */
    uint32_t tx_errors;
    uint32_t rx_errors;
    uint32_t bus_off;
    uint32_t last_error;
    uint32_t last_data_error;
    uint32_t elapsed_us;   /* sender: time to send all frames; receiver: first to last frame */
    uint32_t done;
} load;

static void status(void)
{
    tcan_status_t st;
    tcan_get_status(&st);
    load.rx_lost = st.rx_lost;
    load.rx_overflows = st.rx_overflows;
    load.tx_errors = st.tx_errors;
    load.rx_errors = st.rx_errors;
    load.bus_off = st.bus_off;
    load.last_error = st.last_error;
    load.last_data_error = st.last_data_error;
}

static void fill(uint8_t *data, unsigned len, uint32_t counter)
{
    memcpy(data, &counter, sizeof counter);
    for (unsigned i = sizeof counter; i < len; i++) {
        data[i] = (uint8_t)(counter + i);
    }
}

static void receive(void)
{
    uint32_t expected = 0;
    bool first = true;
    absolute_time_t t_first = get_absolute_time();
    absolute_time_t t_last = t_first;
    absolute_time_t next_status = make_timeout_time_ms(200);
    for (;;) {
        tcan_poll();
        tcan_msg_t msg;
        while (tcan_receive(&msg)) {
            uint32_t counter;
            uint8_t expect[64];
            memcpy(&counter, msg.data, sizeof counter);
            if (msg.id != TX_ID || msg.len < sizeof counter) {
                load.bad++;
                continue;
            }
            fill(expect, msg.len, counter);
            if (memcmp(expect, msg.data, msg.len) != 0) {
                load.bad++;
            }
            if (first) {
                first = false;
                t_first = get_absolute_time();
            } else if (counter != expected) {
                load.gaps += counter - expected;
            }
            expected = counter + 1;
            load.received++;
            t_last = get_absolute_time();
            load.elapsed_us = (uint32_t)absolute_time_diff_us(t_first, t_last);
        }
        if (time_reached(next_status)) {
            next_status = make_timeout_time_ms(200);
            status();
        }
    }
}

static void send(void)
{
    tcan_msg_t tx = {.id = TX_ID, .fd = true, .brs = true, .len = (uint8_t)load.len};
    uint32_t counter = 0;
    absolute_time_t t0 = get_absolute_time();
    while (counter < load.frames) {
        fill(tx.data, tx.len, counter);
        tcan_err_t err = tcan_send(&tx);
        if (err == TCAN_OK) {
            counter++;
            load.sent = counter;
        } else if (err == TCAN_ERR_BUSY) {
            load.send_busy++;
        } else {
            load.send_err++;
            sleep_us(100);
        }
    }
    load.elapsed_us = (uint32_t)absolute_time_diff_us(t0, get_absolute_time());
    sleep_ms(50);
    status();
    load.done = 1;
}

int main(void)
{
    stdio_init_all();
    while (!load.go) {
        tight_loop_contents();
    }

    tcan_config_t cfg = TCAN_CONFIG_DEFAULT;
    cfg.spi.baudrate = load.spi_hz;
    cfg.nominal_bitrate = load.nominal_bps;
    cfg.data_bitrate = load.data_bps;
    load.init_err = tcan_init(&cfg);
    if (load.init_err != TCAN_OK) {
        load.done = 1;
        for (;;) {
            sleep_ms(1000);
        }
    }

    if (load.role == 0) {
        receive();
    } else {
        send();
    }
    for (;;) {
        sleep_ms(1000);
    }
}
