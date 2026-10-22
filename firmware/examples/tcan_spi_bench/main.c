/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Measures the SPI link to the TCAN4551 at several clock rates (#102): the
 * message RAM (2 KB) is written with a pattern and read back, and the time
 * of a read of one 64-byte FD receive element is taken. Per clock:
 *   reads / bad_reads / bad_bytes: 2 KB read-backs at the clock under test of
 *       a pattern written at 2 MHz (read errors only)
 *   rw_iters / rw_bad: pattern written and read back, both at the clock under
 *       test (write and read errors)
 *   elem_us_x100: time of one 72-byte read (a receive element, 8 byte header
 *       and 64 bytes of data), in 1/100 us
 *   spierr: the TCAN's SPI error flag (device interrupt SPIERR) after the run
 * `bench` is read by hil/runner/spi_bench.py.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "TCAN4550.h"
#include "cim/hal.h"

#define MRAM_ADDRESS 0x8000u
#define MRAM_BYTES   2048u
#define CHUNK_WORDS  128u
#define READS        5000u /* 5000 x 2 KB = 10.24 MB per clock */
#define RW_ITERS     1000u
#define ELEM_READS   10000u
#define ELEM_WORDS   18u /* 72 bytes */

#define OP_WRITE_BURST 0x61u
#define OP_READ_BURST  0x41u

static const uint32_t clocks_hz[] = {2000000, 5000000, 8000000, 10000000, 12000000,
                                     14000000, 16000000, 18000000, 20000000, 24000000};
#define N_CLOCKS (sizeof clocks_hz / sizeof clocks_hz[0])

struct result {
    uint32_t requested_hz;
    uint32_t actual_hz;
    uint32_t reads;
    uint32_t bad_reads;
    uint32_t bad_bytes;
    uint32_t read_us;      /* time of the `reads` read-backs */
    uint32_t rw_iters;
    uint32_t rw_bad;
    uint32_t elem_us_x100;
    uint32_t spierr;
};

volatile struct {
    uint32_t done;      /* 1 when all clocks are measured */
    uint32_t current;   /* index of the clock being measured */
    uint32_t id0;       /* TCAN device ID read at 2 MHz, "TCAN" = 0x4E414354 */
    struct result r[N_CLOCKS];
} bench;

static cim_spi_dev_t dev = {
    .spi = CIM_TCAN_SPI,
    .sck_pin = CIM_TCAN_SCLK_PIN,
    .mosi_pin = CIM_TCAN_SDI_PIN,
    .miso_pin = CIM_TCAN_SDO_PIN,
    .cs_pin = CIM_TCAN_nCS_PIN,
    .baudrate = 2000000,
};

static uint8_t expected[MRAM_BYTES];
static uint8_t buf[MRAM_BYTES];

static void header(uint8_t op, uint16_t address, uint8_t words)
{
    const uint8_t hdr[4] = {op, (uint8_t)(address >> 8), (uint8_t)address, words};
    cim_spi_select(&dev);
    cim_spi_write(&dev, hdr, sizeof hdr);
}

static void mram_write(const uint8_t *data, unsigned bytes)
{
    for (unsigned off = 0; off < bytes; off += CHUNK_WORDS * 4) {
        header(OP_WRITE_BURST, MRAM_ADDRESS + off, CHUNK_WORDS);
        cim_spi_write(&dev, data + off, CHUNK_WORDS * 4);
        cim_spi_deselect(&dev);
    }
}

static void mram_read(uint8_t *data, unsigned bytes)
{
    for (unsigned off = 0; off < bytes; off += CHUNK_WORDS * 4) {
        header(OP_READ_BURST, MRAM_ADDRESS + off, CHUNK_WORDS);
        cim_spi_read(&dev, data + off, CHUNK_WORDS * 4);
        cim_spi_deselect(&dev);
    }
}

static void pattern(uint8_t *out, uint32_t seed)
{
    uint32_t x = seed;
    for (unsigned i = 0; i < MRAM_BYTES; i += 4) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        out[i] = (uint8_t)(x >> 24);
        out[i + 1] = (uint8_t)(x >> 16);
        out[i + 2] = (uint8_t)(x >> 8);
        out[i + 3] = (uint8_t)x;
    }
}

static unsigned count_bad(const uint8_t *a, const uint8_t *b)
{
    unsigned bad = 0;
    for (unsigned i = 0; i < MRAM_BYTES; i++) {
        bad += a[i] != b[i];
    }
    return bad;
}

static uint32_t read_u32(uint16_t address)
{
    uint8_t rx[4];
    header(OP_READ_BURST, address, 1);
    cim_spi_read(&dev, rx, sizeof rx);
    cim_spi_deselect(&dev);
    return ((uint32_t)rx[3] << 24) | ((uint32_t)rx[2] << 16) | ((uint32_t)rx[1] << 8) | rx[0];
}

static void measure(unsigned i)
{
    struct result *r = (struct result *)&bench.r[i];
    r->requested_hz = clocks_hz[i];

    /* reference pattern, written at 2 MHz */
    dev.baudrate = 2000000;
    cim_spi_init(&dev);
    pattern(expected, 0x1234567u + i);
    mram_write(expected, MRAM_BYTES);
    TCAN4x5x_Device_ClearSPIERR();

    dev.baudrate = clocks_hz[i];
    r->actual_hz = cim_spi_init(&dev);

    absolute_time_t t0 = get_absolute_time();
    for (unsigned n = 0; n < READS; n++) {
        mram_read(buf, MRAM_BYTES);
        unsigned bad = count_bad(buf, expected);
        if (bad != 0) {
            r->bad_reads++;
            r->bad_bytes += bad;
        }
        r->reads = n + 1;
    }
    r->read_us = (uint32_t)absolute_time_diff_us(t0, get_absolute_time());

    for (unsigned n = 0; n < RW_ITERS; n++) {
        pattern(expected, 0x9E3779B9u * (n + 1) + i);
        mram_write(expected, MRAM_BYTES);
        mram_read(buf, MRAM_BYTES);
        if (count_bad(buf, expected) != 0) {
            r->rw_bad++;
        }
        r->rw_iters = n + 1;
    }

    t0 = get_absolute_time();
    for (unsigned n = 0; n < ELEM_READS; n++) {
        header(OP_READ_BURST, MRAM_ADDRESS, ELEM_WORDS);
        cim_spi_read(&dev, buf, ELEM_WORDS * 4);
        cim_spi_deselect(&dev);
    }
    r->elem_us_x100 = (uint32_t)(absolute_time_diff_us(t0, get_absolute_time()) * 100 / ELEM_READS);

    TCAN4x5x_Device_Interrupts ir;
    TCAN4x5x_Device_ReadInterrupts(&ir);
    r->spierr = ir.SPIERR;

    printf("%lu Hz (actual %lu): %lu/%lu bad reads, %lu/%lu bad write+read, 72 B in %lu.%02lu us, SPIERR %lu\n",
           (unsigned long)r->requested_hz, (unsigned long)r->actual_hz, (unsigned long)r->bad_reads,
           (unsigned long)r->reads, (unsigned long)r->rw_bad, (unsigned long)r->rw_iters,
           (unsigned long)(r->elem_us_x100 / 100), (unsigned long)(r->elem_us_x100 % 100),
           (unsigned long)r->spierr);
}

int main(void)
{
    stdio_init_all();
    cim_gpio_init_out(CIM_LED_B_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_B_PIN, !CIM_LED_ACTIVE_LOW);

    cim_spi_init(&dev);
    cim_gpio_init_out(CIM_TCAN_RST_PIN, false);
    cim_gpio_put(CIM_TCAN_RST_PIN, true);
    cim_delay_us(50);
    cim_gpio_put(CIM_TCAN_RST_PIN, false);
    cim_delay_ms(5);
    bench.id0 = read_u32(0x0000);

    for (unsigned i = 0; i < N_CLOCKS; i++) {
        bench.current = i;
        measure(i);
    }
    bench.done = 1;
    for (;;) {
        sleep_ms(1000);
    }
}
