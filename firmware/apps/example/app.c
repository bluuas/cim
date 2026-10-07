/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Example application: one task that blinks the LED and logs a heartbeat.
 */

#define CIM_LOG_TAG APP_NAME
#include "app.h"
#include "app_config.h"
#include "cim/log.h"

#include "FreeRTOS.h"
#include "task.h"

#include "cim/hal.h"
#include "pico/stdlib.h"

static void app_task(void *arg)
{
    (void)arg;
    cim_gpio_init_out(CIM_LED_B_PIN, CIM_LED_ACTIVE_LOW);

    TickType_t last = xTaskGetTickCount();
    for (uint32_t beat = 1;; beat++) {
        cim_gpio_put(CIM_LED_B_PIN, (beat & 1u) != CIM_LED_ACTIVE_LOW);
        CIM_LOG_INFO("heartbeat %lu", (unsigned long)beat);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(APP_HEARTBEAT_MS));
    }
}

void app_run(void)
{
    CIM_LOG_INFO("%s %s starting", APP_NAME, APP_VERSION);
    xTaskCreate(app_task, "app", 2 * configMINIMAL_STACK_SIZE, NULL, APP_TASK_PRIORITY, NULL);
    vTaskStartScheduler();
    panic("scheduler returned");
}
