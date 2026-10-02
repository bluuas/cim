/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cim/hal.h"

#include "pico/time.h"

void cim_delay_us(uint32_t us)
{
    busy_wait_us_32(us);
}

__attribute__((weak)) void cim_delay_ms(uint32_t ms)
{
    sleep_ms(ms);
}
