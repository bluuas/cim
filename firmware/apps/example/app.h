/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Example application: the template for CIM applications.
 */

#ifndef APP_H
#define APP_H

#define APP_NAME    "example"
#define APP_VERSION "0.1.0"

/** Configuration keys of this application (from CIM_CONFIG_KEY_APP_FIRST, see cim/config.h). */
#define APP_CONFIG_KEY_BOOT_COUNT 0x8000u

/** Create the application's tasks and start the scheduler. Does not return. */
void app_run(void);

#endif /* APP_H */
