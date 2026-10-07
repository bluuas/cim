/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app.h"
#include "cim/log.h"
#include "pico/stdlib.h"

int main(void)
{
    stdio_init_all();
    cim_log_init();
    app_run(); /* starts the scheduler, does not return */
}
