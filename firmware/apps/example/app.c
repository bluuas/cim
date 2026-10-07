/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Example application, the template for CIM applications:
 * - reads the node address and name from the configuration (firmware/config)
 *   and counts boots in an application key
 * - commissioning over USB serial runs in its own task (firmware/commission),
 *   e.g. "set address 5"; the application picks up changes immediately
 * - LED: blinking red while the board has no address, green heartbeat with one
 * - logs a heartbeat (firmware/log); `app_state` is read by the HIL test
 */

#define CIM_LOG_TAG APP_NAME
#include "app.h"
#include "app_config.h"
#include "cim/log.h"

#include "FreeRTOS.h"
#include "task.h"

#include "cim/commission.h"
#include "cim/config.h"
#include "cim/hal.h"
#include "pico/stdlib.h"

volatile struct {
    uint32_t beats;
    uint32_t address;    /* 0: not commissioned */
    uint32_t boot_count; /* stored in the configuration, +1 per boot */
    int32_t config_err;  /* result of storing boot_count */
} app_state;

static void led(bool r, bool g, bool b)
{
    cim_gpio_put(CIM_LED_R_PIN, r != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_G_PIN, g != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_B_PIN, b != CIM_LED_ACTIVE_LOW);
}

static void log_identity(uint8_t address)
{
    char name[33] = "";
    int len = cim_config_get(CIM_CONFIG_KEY_NAME, name, sizeof name - 1);
    name[len > 0 && len < (int)sizeof name ? len : 0] = '\0';

    if (address == 0) {
        CIM_LOG_WARN("not commissioned: connect USB and send 'set address <1..239>'");
    } else {
        CIM_LOG_INFO("node address %u, name '%s'", address, name);
    }
}

static void app_task(void *arg)
{
    (void)arg;
    uint8_t address = cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0);
    log_identity(address);

    /* writes erase a flash sector: fine once per boot, not periodically */
    app_state.boot_count = cim_config_get_u32(APP_CONFIG_KEY_BOOT_COUNT, 0) + 1;
    app_state.config_err = cim_config_set_u32(APP_CONFIG_KEY_BOOT_COUNT, app_state.boot_count);
    if (app_state.config_err != CIM_CONFIG_OK) {
        CIM_LOG_ERROR("storing the boot count failed (%ld)", (long)app_state.config_err);
    } else {
        CIM_LOG_INFO("boot %lu", (unsigned long)app_state.boot_count);
    }

    TickType_t last = xTaskGetTickCount();
    for (;;) {
        /* the config store needs no initialisation and is cheap to read */
        uint8_t now = cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0);
        if (now != address) {
            address = now;
            log_identity(address);
        }
        app_state.address = address;
        app_state.beats++;

        bool on = (app_state.beats & 1u) != 0;
        led(address == 0 && on, address != 0 && on, false);
        CIM_LOG_INFO("heartbeat %lu, address %u", (unsigned long)app_state.beats, address);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(APP_HEARTBEAT_MS));
    }
}

static void commission_task(void *arg)
{
    (void)arg;
    for (;;) {
        cim_commission_poll();
        vTaskDelay(pdMS_TO_TICKS(APP_COMMISSION_POLL_MS));
    }
}

void app_run(void)
{
    cim_gpio_init_out(CIM_LED_R_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_G_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_B_PIN, CIM_LED_ACTIVE_LOW);
    led(false, false, false);

    CIM_LOG_INFO("%s %s starting", APP_NAME, APP_VERSION);
    xTaskCreate(app_task, "app", 2 * configMINIMAL_STACK_SIZE, NULL, APP_TASK_PRIORITY, NULL);
    xTaskCreate(commission_task, "commission", 2 * configMINIMAL_STACK_SIZE, NULL, APP_COMMISSION_PRIORITY, NULL);
    vTaskStartScheduler();
    panic("scheduler returned");
}
