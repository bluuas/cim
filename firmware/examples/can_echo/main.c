/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Echoes every received frame with ID + 1 and the same payload and flags.
 * The partner is examples/can_tx_counter; `state` is read by
 * hil/runner/test_can_echo.py.
 * LED: green toggles every 250 frames, red = bus-off or send error,
 *      yellow = TCAN not initialised (no VSUP: CAN needs +BATT on the mezzanine).
 * The counters are also printed over USB CDC once a second.
 */

#include <stdio.h>

#include "cim/hal.h"
#include "pico/stdlib.h"
#include "tcan.h"
#include "TCAN4550.h"

volatile struct {
    int32_t init_err;      /* tcan_init() result, 0 = OK, -4 = no VSUP */
    uint32_t received;
    uint32_t echoed;
    uint32_t send_busy;    /* retries because all TX buffers were pending */
    uint32_t send_err;     /* echo dropped: still busy after the retries, or bus-off */
    uint32_t last_id;
    uint32_t tx_errors;    /* tcan_get_status(), once a second */
    uint32_t rx_errors;
    uint32_t bus_off;
    uint32_t rx_lost;
    uint32_t rx_overflows;
    uint32_t last_error;   /* MCAN last error code, arbitration phase (tcan_status_t) */
    uint32_t last_data_error; /* same, data phase */
    uint32_t dev_ir;       /* TCAN device interrupt register 0x0820: bus fault flags in bits 8, 10, 24..31 */
} state;

static void led(bool r, bool g, bool b)
{
    cim_gpio_put(CIM_LED_R_PIN, r != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_G_PIN, g != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_B_PIN, b != CIM_LED_ACTIVE_LOW);
}

static void echo(tcan_msg_t *msg)
{
    msg->id = (msg->id + 1) & (msg->ext ? 0x1FFFFFFFu : 0x7FFu);
    tcan_err_t err;
    for (unsigned tries = 0; tries < 100; tries++) {
        err = tcan_send(msg);
        if (err != TCAN_ERR_BUSY) {
            break;
        }
        state.send_busy++;
        cim_delay_us(50); /* a 64-byte FD frame takes about 300 us at 500k/2M */
    }
    if (err == TCAN_OK) {
        state.echoed++;
    } else {
        state.send_err++;
    }
}

int main(void)
{
    stdio_init_all();
    cim_gpio_init_out(CIM_LED_R_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_G_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_B_PIN, CIM_LED_ACTIVE_LOW);
    led(false, false, true);

    tcan_config_t cfg = TCAN_CONFIG_DEFAULT; /* 500 kbit/s, 2 Mbit/s data, accept all */
    state.init_err = tcan_init(&cfg);
    if (state.init_err != TCAN_OK) {
        led(true, true, false);
        for (;;) {
            printf("can_echo: tcan_init failed (%ld)\n", (long)state.init_err);
            sleep_ms(1000);
        }
    }

    bool green = false;
    absolute_time_t next_report = make_timeout_time_ms(1000);
    for (;;) {
        tcan_poll();
        tcan_msg_t msg;
        while (tcan_receive(&msg)) {
            state.received++;
            state.last_id = msg.id;
            echo(&msg);
            if (state.received % 250 == 0) {
                green = !green;
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
            printf("can_echo: received=%lu echoed=%lu busy=%lu err=%lu lost=%lu tec=%lu rec=%lu%s\n",
                   (unsigned long)state.received, (unsigned long)state.echoed, (unsigned long)state.send_busy,
                   (unsigned long)state.send_err, (unsigned long)state.rx_lost, (unsigned long)state.tx_errors,
                   (unsigned long)state.rx_errors, st.bus_off ? " BUS-OFF" : "");
        }
        bool bad = state.bus_off || state.send_err > 0;
        led(bad, !bad && green, false);
    }
}
