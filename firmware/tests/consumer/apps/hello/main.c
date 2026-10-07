/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Smallest app of an external project: one FreeRTOS task that logs.
 */

#define CIM_LOG_TAG "hello"
#include "cim/log.h"

#include "FreeRTOS.h"
#include "task.h"

#include "pico/stdlib.h"

static void hello_task(void *arg)
{
    (void)arg;
    for (;;) {
        CIM_LOG_INFO("hello from outside");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    stdio_init_all();
    cim_log_init();
    xTaskCreate(hello_task, "hello", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();
}
