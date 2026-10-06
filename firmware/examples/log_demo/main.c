/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Shows cim/log.h: logs a counter every 100 ms on all levels. With the default
 * CIM_LOG_LEVEL (INFO), the DEBUG lines are compiled out. Read the output over
 * USB CDC or over SWD with RTT (see firmware/log/README.md).
 */

#define CIM_LOG_TAG "demo"
#include "cim/log.h"

#include "pico/stdlib.h"

int main(void)
{
    stdio_init_all();
    cim_log_init();

    for (uint32_t i = 0;; i++) {
        CIM_LOG_INFO("count %lu", (unsigned long)i);
        if (i % 10 == 0) {
            CIM_LOG_WARN("count %lu is a multiple of 10", (unsigned long)i);
        }
        CIM_LOG_DEBUG("count %lu (debug, compiled out by default)", (unsigned long)i);
        sleep_ms(100);
    }
}
