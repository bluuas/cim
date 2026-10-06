/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Pico SDK implementation of the commissioning platform hooks.
 */

#include <stdio.h>

#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include "cim/commission.h"

int cim_commission_getchar(void)
{
    int c = getchar_timeout_us(0);
    return c == PICO_ERROR_TIMEOUT ? -1 : c;
}

void cim_commission_get_uid(uint8_t uid[8])
{
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    for (int i = 0; i < 8; i++) {
        uid[i] = id.id[i];
    }
}

void cim_commission_reboot(int bootsel)
{
    printf("ok rebooting%s\n", bootsel ? " into USB bootloader" : "");
    stdio_flush();
    sleep_ms(50); /* let USB send the answer */
    if (bootsel) {
        reset_usb_boot(0, 0);
    }
    watchdog_reboot(0, 0, 0);
    for (;;) {
    }
}
