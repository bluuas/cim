/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Host tests for the log line formatting and the compile-time filter.
 */

#define CIM_LOG_LEVEL CIM_LOG_LEVEL_WARN
#define CIM_LOG_TAG   "test"
#include "cim/log.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* platform mock: fixed clock, last line captured */
static uint32_t now_ms;
static char out[256];
static size_t out_len;
static int out_count;

uint32_t cim_log_time_ms(void)
{
    return now_ms;
}

void cim_log_output(const char *line, size_t len)
{
    assert(len < sizeof out);
    memcpy(out, line, len);
    out[len] = '\0';
    out_len = len;
    out_count++;
}

static int side_effects;

static int touch(void)
{
    return ++side_effects;
}

static void test_format(void)
{
    now_ms = 12345;
    cim_log_write(CIM_LOG_LEVEL_INFO, "tcan", "bus on, %u kbit/s", 500u);
    assert(strcmp(out, "[    12.345] I tcan: bus on, 500 kbit/s\n") == 0);

    cim_log_write(CIM_LOG_LEVEL_ERROR, "", "no tag");
    assert(strcmp(out, "[    12.345] E no tag\n") == 0);

    cim_log_write(CIM_LOG_LEVEL_DEBUG, NULL, "null tag");
    assert(strcmp(out, "[    12.345] D null tag\n") == 0);

    cim_log_write(7, "x", "bad level");
    assert(strcmp(out, "[    12.345] ? x: bad level\n") == 0);

    now_ms = 0;
    cim_log_write(CIM_LOG_LEVEL_WARN, "a", "%s", "");
    assert(strcmp(out, "[     0.000] W a: \n") == 0);
}

static void test_truncation(void)
{
    char longmsg[300];
    memset(longmsg, 'x', sizeof longmsg - 1);
    longmsg[sizeof longmsg - 1] = '\0';

    cim_log_write(CIM_LOG_LEVEL_INFO, "t", "%s", longmsg);
    assert(out_len == CIM_LOG_LINE_MAX);
    assert(out[out_len - 1] == '\n');
    assert(memcmp(out + out_len - 4, "...", 3) == 0);

    /* exactly fitting: no "..." */
    char fit[CIM_LOG_LINE_MAX];
    size_t prefix = strlen("[     0.000] I t: ");
    size_t room = CIM_LOG_LINE_MAX - 1 - prefix;
    memset(fit, 'y', room);
    fit[room] = '\0';
    now_ms = 0;
    cim_log_write(CIM_LOG_LEVEL_INFO, "t", "%s", fit);
    assert(out_len == CIM_LOG_LINE_MAX);
    assert(out[out_len - 2] == 'y');
}

static void test_filter(void)
{
    /* CIM_LOG_LEVEL is WARN in this file */
    out_count = 0;
    side_effects = 0;
    CIM_LOG_ERROR("e %d", touch());
    CIM_LOG_WARN("w %d", touch());
    CIM_LOG_INFO("i %d", touch());
    CIM_LOG_DEBUG("d %d", touch());
    assert(out_count == 2);
    assert(side_effects == 2); /* filtered calls do not evaluate their arguments */
    assert(strstr(out, "W test: w 2") != NULL);
}

int main(void)
{
    test_format();
    test_truncation();
    test_filter();
    printf("log: all tests passed\n");
    return 0;
}
