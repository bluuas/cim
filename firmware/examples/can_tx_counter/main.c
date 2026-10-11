/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Sends a CAN FD frame with a counter every PERIOD_US and checks the echoes
 * that examples/can_echo sends back with ID + 1. Together they measure frame
 * loss on a bus: `state` is read by hil/runner/test_can_echo.py.
 * LED: green toggles every 250 frames, red = bus-off or send error,
 *      yellow = TCAN not initialised (no VSUP: CAN needs +BATT on the mezzanine).
 * The result is also printed over USB CDC once a second.
 */

#include <stdio.h>
#include <string.h>

#include "cim/hal.h"
#include "pico/stdlib.h"
#include "tcan.h"
#include "TCAN4550.h"

#define TX_ID     0x100u
#define PERIOD_US 2000u
#define LEN       64u

volatile struct {
    int32_t init_err;      /* tcan_init() result, 0 = OK, -4 = no VSUP */
    uint32_t sent;
    uint32_t send_busy;    /* tcan_send() returned TCAN_ERR_BUSY: frame skipped */
    uint32_t send_err;     /* other tcan_send() errors (bus-off) */
    uint32_t echoes;       /* ID + 1 frames with the expected payload */
    uint32_t echo_bad;     /* ID + 1 frames with a wrong length or payload */
    uint32_t echo_gaps;    /* counter jumps between consecutive echoes: frames lost */
    uint32_t other_rx;     /* frames with another ID */
    uint32_t period_us;
    uint32_t len;
    uint32_t tx_errors;    /* tcan_get_status(), once a second */
    uint32_t rx_errors;
    uint32_t bus_off;
    uint32_t rx_lost;
    uint32_t rx_overflows;
    uint32_t last_error;   /* MCAN last error code, arbitration phase (tcan_status_t) */
    uint32_t last_data_error; /* same, data phase */
    uint32_t dev_ir;       /* TCAN device interrupt register 0x0820: bus fault flags in bits 8, 10, 24..31 */
} state = {.period_us = PERIOD_US, .len = LEN};

static void led(bool r, bool g, bool b)
{
    cim_gpio_put(CIM_LED_R_PIN, r != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_G_PIN, g != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_B_PIN, b != CIM_LED_ACTIVE_LOW);
}

/* payload: counter (little endian), then a pattern derived from it */
static void fill(uint8_t *data, uint32_t counter)
{
    memcpy(data, &counter, sizeof counter);
    for (unsigned i = sizeof counter; i < LEN; i++) {
        data[i] = (uint8_t)(counter + i);
    }
}

static void check_echo(const tcan_msg_t *msg, uint32_t *last)
{
    uint8_t expected[LEN];
    uint32_t counter;
    if (msg->len != LEN) {
        state.echo_bad++;
        return;
    }
    memcpy(&counter, msg->data, sizeof counter);
    fill(expected, counter);
    if (memcmp(expected, msg->data, LEN) != 0) {
        state.echo_bad++;
        return;
    }
    if (state.echoes > 0 && counter != *last + 1) {
        state.echo_gaps++;
    }
    *last = counter;
    state.echoes++;
}

int main(void)
{
    stdio_init_all();
    cim_gpio_init_out(CIM_LED_R_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_G_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_B_PIN, CIM_LED_ACTIVE_LOW);
    led(false, false, true);

    tcan_config_t cfg = TCAN_CONFIG_DEFAULT; /* 500 kbit/s, 2 Mbit/s data */
    state.init_err = tcan_init(&cfg);
    if (state.init_err != TCAN_OK) {
        led(true, true, false);
        for (;;) {
            printf("can_tx_counter: tcan_init failed (%ld)\n", (long)state.init_err);
            sleep_ms(1000);
        }
    }

    tcan_msg_t tx = {.id = TX_ID, .fd = true, .brs = true, .len = LEN};
    uint32_t counter = 0, last_echo = 0;
    bool green = false;
    absolute_time_t next_tx = get_absolute_time();
    absolute_time_t next_report = make_timeout_time_ms(1000);

    for (;;) {
        if (time_reached(next_tx)) {
            next_tx = delayed_by_us(next_tx, PERIOD_US);
            fill(tx.data, counter);
            tcan_err_t err = tcan_send(&tx);
            if (err == TCAN_OK) {
                state.sent++;
                counter++;
                if (state.sent % 250 == 0) {
                    green = !green;
                }
            } else if (err == TCAN_ERR_BUSY) {
                state.send_busy++;
            } else {
                state.send_err++;
            }
        }

        tcan_poll();
        tcan_msg_t rx;
        while (tcan_receive(&rx)) {
            if (rx.id == TX_ID + 1) {
                check_echo(&rx, &last_echo);
            } else {
                state.other_rx++;
            }
        }

        if (time_reached(next_report)) {
            next_report = delayed_by_us(next_report, 1000000);
            tcan_status_t st;
            tcan_get_status(&st);
            state.tx_errors = st.tx_errors;
            state.rx_errors = st.rx_errors;
            state.bus_off = st.bus_off;
            state.rx_lost = st.rx_lost;
            state.rx_overflows = st.rx_overflows;
            state.last_error = st.last_error;
            state.last_data_error = st.last_data_error;
            TCAN4x5x_Device_Interrupts ir;
            TCAN4x5x_Device_ReadInterrupts(&ir);
            state.dev_ir = ir.word;
            printf("can_tx_counter: sent=%lu echoes=%lu bad=%lu gaps=%lu busy=%lu err=%lu tec=%lu rec=%lu%s\n",
                   (unsigned long)state.sent, (unsigned long)state.echoes, (unsigned long)state.echo_bad,
                   (unsigned long)state.echo_gaps, (unsigned long)state.send_busy, (unsigned long)state.send_err,
                   (unsigned long)state.tx_errors, (unsigned long)state.rx_errors, st.bus_off ? " BUS-OFF" : "");
        }
        bool bad = state.bus_off || state.send_err > 0;
        led(bad, !bad && green, false);
    }
}
