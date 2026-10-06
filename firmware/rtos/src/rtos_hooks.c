/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * FreeRTOS hooks and the RTOS-aware cim_delay_ms(). Compiled into every
 * application that links cim_rtos (INTERFACE sources), so the strong
 * cim_delay_ms() here replaces the weak busy-wait one from cim_hal.
 */

#define CIM_LOG_TAG "rtos"
#include "cim/log.h"

#include "FreeRTOS.h"
#include "task.h"

#include "cim/hal.h"
#include "pico/stdlib.h"

void cim_delay_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        /* round up: never return earlier than asked */
        vTaskDelay((ms + portTICK_PERIOD_MS - 1u) / portTICK_PERIOD_MS);
    } else {
        sleep_ms(ms);
    }
}

void vApplicationMallocFailedHook(void)
{
    CIM_LOG_ERROR("out of heap (configTOTAL_HEAP_SIZE %u, free %u)", (unsigned)configTOTAL_HEAP_SIZE,
                  (unsigned)xPortGetFreeHeapSize());
    panic("FreeRTOS: malloc failed");
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    CIM_LOG_ERROR("stack overflow in task '%s'", name);
    panic("FreeRTOS: stack overflow in %s", name);
}
