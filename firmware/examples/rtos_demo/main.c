/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Checks the FreeRTOS integration (cim_rtos) on hardware:
 * - "fast" (high priority) waits with cim_delay_ms(10) and counts.
 * - "spin" (low priority) never blocks and counts as fast as it can. It only
 *   runs while "fast" waits, so its counter shows that cim_delay_ms() blocks
 *   the task instead of busy-waiting.
 * - "led" blinks green every 500 ms and logs a heartbeat.
 * The counters are kept in `state` for the HIL test (hil/runner/test_rtos.py).
 */

#define CIM_LOG_TAG "rtos_demo"
#include "cim/log.h"

#include "FreeRTOS.h"
#include "task.h"

#include "cim/hal.h"
#include "pico/stdlib.h"

volatile struct {
    uint32_t fast_count;
    uint32_t spin_count;
    uint32_t led_count;
    uint32_t tick_hz;
    uint32_t free_heap;
} state;

static void fast_task(void *arg)
{
    (void)arg;
    for (;;) {
        cim_delay_ms(10);
        state.fast_count++;
    }
}

static void spin_task(void *arg)
{
    (void)arg;
    for (;;) {
        state.spin_count++;
    }
}

static void led_task(void *arg)
{
    (void)arg;
    cim_gpio_init_out(CIM_LED_G_PIN, CIM_LED_ACTIVE_LOW);
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        cim_gpio_put(CIM_LED_G_PIN, (state.led_count & 1u) != CIM_LED_ACTIVE_LOW);
        state.led_count++;
        state.free_heap = xPortGetFreeHeapSize();
        CIM_LOG_INFO("heartbeat %lu: fast %lu, spin %lu, free heap %lu", (unsigned long)state.led_count,
                     (unsigned long)state.fast_count, (unsigned long)state.spin_count,
                     (unsigned long)state.free_heap);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    stdio_init_all();
    cim_log_init();
    state.tick_hz = configTICK_RATE_HZ;
    CIM_LOG_INFO("starting FreeRTOS %s", tskKERNEL_VERSION_NUMBER);

    xTaskCreate(fast_task, "fast", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    xTaskCreate(led_task, "led", 2 * configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(spin_task, "spin", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();

    panic("scheduler returned");
}
