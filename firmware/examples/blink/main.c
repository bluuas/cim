/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Cycles the RGB LED through its colours and prints a heartbeat over USB CDC.
 */

#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"

static const uint led_pins[] = {CIM_LED_R_PIN, CIM_LED_G_PIN, CIM_LED_B_PIN};
#define NUM_LEDS (sizeof(led_pins) / sizeof(led_pins[0]))

static void led_set(uint pin, bool on)
{
    gpio_put(pin, CIM_LED_ACTIVE_LOW ? !on : on);
}

int main(void)
{
    stdio_init_all();

    for (uint i = 0; i < NUM_LEDS; i++) {
        gpio_init(led_pins[i]);
        gpio_set_dir(led_pins[i], GPIO_OUT);
        led_set(led_pins[i], false);
    }

    /* bit 0 = red, bit 1 = green, bit 2 = blue */
    for (uint32_t count = 0;; count++) {
        uint32_t colour = (count % 7) + 1;
        for (uint i = 0; i < NUM_LEDS; i++) {
            led_set(led_pins[i], colour & (1u << i));
        }
        printf("blink %lu\n", (unsigned long)count);
        sleep_ms(500);
    }
}
