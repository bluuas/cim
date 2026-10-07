/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Build-time settings of this application. Also read by FreeRTOSConfig.h,
 * which takes the values defined here instead of its defaults, e.g.:
 *
 *   #define configTOTAL_HEAP_SIZE (32 * 1024)
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/** Priority of the application task (0 = idle, configMAX_PRIORITIES - 1 = highest). */
#define APP_TASK_PRIORITY 2

/** Heartbeat period of the LED and the log line. */
#define APP_HEARTBEAT_MS 1000

#endif /* APP_CONFIG_H */
