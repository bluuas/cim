/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cim/hal.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"

void cim_gpio_init_out(uint8_t pin, bool value)
{
    gpio_init(pin);
    gpio_put(pin, value);
    gpio_set_dir(pin, GPIO_OUT);
}

void cim_gpio_init_in(uint8_t pin, cim_gpio_pull_t pull)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_pulls(pin, pull == CIM_GPIO_PULL_UP, pull == CIM_GPIO_PULL_DOWN);
}

void cim_gpio_put(uint8_t pin, bool value)
{
    gpio_put(pin, value);
}

bool cim_gpio_get(uint8_t pin)
{
    return gpio_get(pin);
}

/*
 * Interrupts: one raw IRQ handler (dispatch) is registered for the set of pins
 * that have a callback. Raw handlers coexist with gpio_set_irq_callback(),
 * which keeps handling all other pins.
 */

static struct {
    cim_gpio_irq_cb_t cb;
    void *ctx;
    uint32_t events;
} irq_slots[NUM_BANK0_GPIOS];

static uint32_t irq_pin_mask; /* pins currently handled by dispatch() */

static void dispatch(void)
{
    uint32_t mask = irq_pin_mask;
    while (mask) {
        uint pin = __builtin_ctz(mask);
        mask &= mask - 1;
        uint32_t events = gpio_get_irq_event_mask(pin) & irq_slots[pin].events;
        if (events) {
            gpio_acknowledge_irq(pin, events);
            irq_slots[pin].cb((uint8_t)pin, irq_slots[pin].ctx);
        }
    }
}

static void set_pin_mask(uint32_t new_mask)
{
    if (irq_pin_mask) {
        gpio_remove_raw_irq_handler_masked(irq_pin_mask, dispatch);
    }
    irq_pin_mask = new_mask;
    if (irq_pin_mask) {
        gpio_add_raw_irq_handler_masked(irq_pin_mask, dispatch);
    }
}

bool cim_gpio_irq_enable(uint8_t pin, unsigned edges, cim_gpio_irq_cb_t cb, void *ctx)
{
    if (pin >= NUM_BANK0_GPIOS || cb == NULL) {
        return false;
    }
    uint32_t events = 0;
    if (edges & CIM_GPIO_EDGE_FALL) {
        events |= GPIO_IRQ_EDGE_FALL;
    }
    if (edges & CIM_GPIO_EDGE_RISE) {
        events |= GPIO_IRQ_EDGE_RISE;
    }

    uint32_t save = save_and_disable_interrupts();
    irq_slots[pin].cb = cb;
    irq_slots[pin].ctx = ctx;
    irq_slots[pin].events = events;
    set_pin_mask(irq_pin_mask | (1u << pin));
    restore_interrupts(save);

    gpio_acknowledge_irq(pin, events);
    gpio_set_irq_enabled(pin, events, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
    return true;
}

void cim_gpio_irq_disable(uint8_t pin)
{
    if (pin >= NUM_BANK0_GPIOS) {
        return;
    }
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, false);

    uint32_t save = save_and_disable_interrupts();
    set_pin_mask(irq_pin_mask & ~(1u << pin));
    irq_slots[pin].cb = NULL;
    irq_slots[pin].ctx = NULL;
    irq_slots[pin].events = 0;
    restore_interrupts(save);
}
