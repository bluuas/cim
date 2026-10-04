/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Checks the configuration store on hardware: counts boots in a config key and
 * shows the node address. After each reset or power cycle the boot counter
 * must have increased by one.
 * LED: blinks green boot_count times (max 10), then stays blue.
 * The values are printed over USB CDC and kept in `state` for a debugger.
 */

#include <stdio.h>

#include "pico/stdlib.h"

#include "cim/config.h"
#include "cim/hal.h"

#define KEY_BOOT_COUNT CIM_CONFIG_KEY_APP_FIRST

volatile struct {
    uint32_t boot_count;
    uint32_t address; /* 0: not configured */
    int32_t last_err;
} state;

static void led(bool r, bool g, bool b)
{
    cim_gpio_put(CIM_LED_R_PIN, r != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_G_PIN, g != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_B_PIN, b != CIM_LED_ACTIVE_LOW);
}

int main(void)
{
    stdio_init_all();
    cim_gpio_init_out(CIM_LED_R_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_G_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_B_PIN, CIM_LED_ACTIVE_LOW);

    state.boot_count = cim_config_get_u32(KEY_BOOT_COUNT, 0) + 1;
    state.last_err = cim_config_set_u32(KEY_BOOT_COUNT, state.boot_count);
    state.address = cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0);

    if (state.last_err != CIM_CONFIG_OK) {
        led(true, false, false);
    } else {
        for (uint32_t i = 0; i < state.boot_count && i < 10; i++) {
            led(false, true, false);
            sleep_ms(150);
            led(false, false, false);
            sleep_ms(250);
        }
        led(false, false, true);
    }

    for (;;) {
        printf("config_counter: boot %lu, address %lu, err %ld\n", (unsigned long)state.boot_count,
               (unsigned long)state.address, (long)state.last_err);
        sleep_ms(1000);
    }
}
