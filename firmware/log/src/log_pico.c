/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Outputs for cim/log.h on the RP2040/RP2350: USB CDC through pico_stdio and
 * an RTT up-buffer.
 *
 * RTT ("Real Time Transfer") is a ring buffer in RAM that a debugger reads
 * over SWD while the target runs. This is a minimal implementation of the
 * control block layout that debuggers search for (ID "SEGGER RTT"): one up
 * buffer for the log, one empty down buffer. When the buffer is full, the
 * line is dropped, so logging never blocks without a debugger. With OpenOCD:
 *
 *   rtt setup <address of cim_log_rtt> 16 "SEGGER RTT"
 *   rtt start
 *   rtt server start 9090 0      (then read TCP port 9090)
 */

#include "cim/log.h"

#include <string.h>

#include "hardware/sync.h"
#include "pico/critical_section.h"
#include "pico/stdio.h"
#include "pico/time.h"

#ifndef CIM_LOG_USB
#define CIM_LOG_USB 1 /**< also write to pico_stdio (USB CDC if enabled by the application) */
#endif

#ifndef CIM_LOG_RTT_BUFFER_SIZE
#define CIM_LOG_RTT_BUFFER_SIZE 1024u
#endif

typedef struct {
    const char *name;
    char *buffer;
    uint32_t size;
    volatile uint32_t wr_off; /* written by the target */
    volatile uint32_t rd_off; /* written by the debugger */
    uint32_t flags;           /* 0: skip (drop data when full) */
} rtt_ring_t;

typedef struct {
    char id[16];
    int32_t max_up;
    int32_t max_down;
    rtt_ring_t up[1];
    rtt_ring_t down[1];
} rtt_cb_t;

/* Non-static so a debugger can find it by symbol instead of searching RAM. */
rtt_cb_t cim_log_rtt;

static char rtt_up_buf[CIM_LOG_RTT_BUFFER_SIZE];
static char rtt_down_buf[16];
static critical_section_t lock;
static bool ready;

void cim_log_init(void)
{
    if (ready) {
        return;
    }
    critical_section_init(&lock);

    cim_log_rtt.max_up = 1;
    cim_log_rtt.max_down = 1;
    cim_log_rtt.up[0] = (rtt_ring_t){.name = "Terminal", .buffer = rtt_up_buf, .size = sizeof rtt_up_buf};
    cim_log_rtt.down[0] = (rtt_ring_t){.name = "Terminal", .buffer = rtt_down_buf, .size = sizeof rtt_down_buf};
    /* write the ID last: once a debugger finds it, the block must be complete */
    __dmb();
    memcpy(cim_log_rtt.id, "SEGGER RTT\0\0\0\0\0", sizeof cim_log_rtt.id);
    __dmb();
    ready = true;
}

uint32_t cim_log_time_ms(void)
{
    return to_ms_since_boot(get_absolute_time());
}

static void rtt_write(const char *data, size_t len)
{
    rtt_ring_t *r = &cim_log_rtt.up[0];
    uint32_t wr = r->wr_off;
    uint32_t rd = r->rd_off;
    uint32_t free = (rd > wr) ? rd - wr - 1u : r->size - (wr - rd) - 1u;
    if (len > free) {
        return; /* no debugger reading, or too slow: drop the line */
    }
    for (size_t i = 0; i < len; i++) {
        r->buffer[wr] = data[i];
        wr = (wr + 1u == r->size) ? 0u : wr + 1u;
    }
    __dmb(); /* data before the write offset */
    r->wr_off = wr;
}

void cim_log_output(const char *line, size_t len)
{
    if (!ready) {
        return;
    }
    critical_section_enter_blocking(&lock);
    rtt_write(line, len);
    critical_section_exit(&lock);

#if CIM_LOG_USB
    /* pico_stdio serialises writers itself; '\n' becomes "\r\n" */
    stdio_put_string(line, (int)len, false, true);
#endif
}
