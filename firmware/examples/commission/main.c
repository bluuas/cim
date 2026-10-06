/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Commissioning a new board: connect USB, open the serial port and type
 * "set address 5" (see cim/commission.h for all commands).
 * LED: green = address configured, blinking red = no address yet.
 */

#include "pico/stdlib.h"

#include "cim/commission.h"
#include "cim/config.h"
#include "cim/hal.h"

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

    for (uint32_t tick = 0;; tick++) {
        cim_commission_poll();
        bool configured = cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0) != 0;
        led(!configured && (tick / 25) % 2, configured, false);
        sleep_ms(10);
    }
}
