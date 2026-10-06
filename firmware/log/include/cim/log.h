/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Logging with levels and a compile-time filter.
 *
 *   #define CIM_LOG_TAG "tcan"   // optional, before the include
 *   #include "cim/log.h"
 *
 *   cim_log_init();             // once at startup
 *   CIM_LOG_INFO("bus on, %u kbit/s", rate);
 *
 * gives the line
 *
 *   [    12.345] I tcan: bus on, 500 kbit/s
 *
 * Calls below CIM_LOG_LEVEL are removed by the compiler; their arguments are
 * still type-checked. Each line is formatted into a buffer and written in one
 * piece, so lines from different tasks do not interleave. Longer lines are
 * cut at CIM_LOG_LINE_MAX and end in "...".
 *
 * On the CIM, lines go to USB CDC (pico_stdio, if the application enables
 * stdio over USB) and to an RTT buffer that a debugger reads over SWD, e.g.
 * OpenOCD's "rtt" commands. Do not log from interrupt handlers.
 */

#ifndef CIM_LOG_H
#define CIM_LOG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------- Levels --------------------------------- */

#define CIM_LOG_LEVEL_NONE  0
#define CIM_LOG_LEVEL_ERROR 1
#define CIM_LOG_LEVEL_WARN  2
#define CIM_LOG_LEVEL_INFO  3
#define CIM_LOG_LEVEL_DEBUG 4

/** Highest level that is compiled in. Set per target, e.g. with target_compile_definitions(). */
#ifndef CIM_LOG_LEVEL
#define CIM_LOG_LEVEL CIM_LOG_LEVEL_INFO
#endif

/** Module name printed in each line; empty: no tag. */
#ifndef CIM_LOG_TAG
#define CIM_LOG_TAG ""
#endif

/** Maximum line length including the newline. */
#ifndef CIM_LOG_LINE_MAX
#define CIM_LOG_LINE_MAX 128
#endif

/* --------------------------------- Macros --------------------------------- */

#define CIM_LOG_AT(level, ...)                                                                              \
    do {                                                                                                     \
        if (CIM_LOG_LEVEL >= (level)) {                                                                      \
            cim_log_write((level), CIM_LOG_TAG, __VA_ARGS__);                                                \
        }                                                                                                    \
    } while (0)

#define CIM_LOG_ERROR(...) CIM_LOG_AT(CIM_LOG_LEVEL_ERROR, __VA_ARGS__)
#define CIM_LOG_WARN(...)  CIM_LOG_AT(CIM_LOG_LEVEL_WARN, __VA_ARGS__)
#define CIM_LOG_INFO(...)  CIM_LOG_AT(CIM_LOG_LEVEL_INFO, __VA_ARGS__)
#define CIM_LOG_DEBUG(...) CIM_LOG_AT(CIM_LOG_LEVEL_DEBUG, __VA_ARGS__)

/* ---------------------------------- API ----------------------------------- */

/** Set up the outputs (RTT buffer). Lines logged before are dropped. */
void cim_log_init(void);

/** Format and output one line. Use the macros instead, they apply the compile-time filter. */
void cim_log_write(int level, const char *tag, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

/* ------------------------- Platform (log_pico.c) -------------------------- */

/** Milliseconds since boot, for the timestamp. */
uint32_t cim_log_time_ms(void);

/** Write one complete line (ends with '\n', not NUL-terminated) to all outputs. */
void cim_log_output(const char *line, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CIM_LOG_H */
