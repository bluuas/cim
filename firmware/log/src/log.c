/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Line formatting for cim/log.h. Platform-independent (host-tested); the
 * outputs and the clock are in log_pico.c.
 */

#include "cim/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const char level_char[] = {'-', 'E', 'W', 'I', 'D'};

void cim_log_write(int level, const char *tag, const char *fmt, ...)
{
    char line[CIM_LOG_LINE_MAX];
    const size_t max = sizeof line - 1; /* room for '\n' */

    uint32_t ms = cim_log_time_ms();
    char c = (level >= CIM_LOG_LEVEL_ERROR && level <= CIM_LOG_LEVEL_DEBUG) ? level_char[level] : '?';
    int n = snprintf(line, sizeof line, "[%6lu.%03lu] %c %s%s", (unsigned long)(ms / 1000u),
                     (unsigned long)(ms % 1000u), c, (tag && tag[0]) ? tag : "", (tag && tag[0]) ? ": " : "");
    size_t len = (n < 0) ? 0 : ((size_t)n < max ? (size_t)n : max);

    if (len < max) {
        va_list ap;
        va_start(ap, fmt);
        n = vsnprintf(line + len, sizeof line - len, fmt, ap);
        va_end(ap);
        if (n > 0) {
            len += (size_t)n;
        }
    }

    if (len > max) {
        /* truncated: mark the cut */
        len = max;
        memcpy(line + len - 3, "...", 3);
    }
    line[len++] = '\n';
    cim_log_output(line, len);
}
