/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Checks the HAL against the TCAN4x5x: reset via GPIO, read the device ID over
 * SPI, and count nINT interrupts (the device signals power-on after reset).
 * LED: green = ID read correctly and power-on interrupt seen on nINT,
 *      yellow = OK but VSUP (12 V) missing (UVSUP, keeps nINT asserted),
 *      red = failure.
 * The result is also printed over USB CDC and kept in `probe` for a debugger.
 */

#include <stdio.h>

#include "pico/stdlib.h"

#include "cim/hal.h"

#define AHB_READ_OPCODE    0x41
#define AHB_WRITE_OPCODE   0x61
#define REG_SPI_DEVICE_ID0 0x0000
#define REG_SPI_DEVICE_ID1 0x0004
#define REG_SPI_REVISION   0x0008
#define REG_SPI_STATUS     0x000C
#define REG_DEV_IR         0x0820
#define REG_MCAN_IR        0x1050

#define DEVICE_ID0_TCAN 0x4E414354u /* "TCAN", LSB first */
#define DEV_IR_PWRON    (1u << 20)
#define DEV_IR_UVSUP    (1u << 22)

static const cim_spi_dev_t tcan_spi = {
    .spi = CIM_TCAN_SPI,
    .sck_pin = CIM_TCAN_SCLK_PIN,
    .mosi_pin = CIM_TCAN_SDI_PIN,
    .miso_pin = CIM_TCAN_SDO_PIN,
    .cs_pin = CIM_TCAN_nCS_PIN,
    .baudrate = 2 * 1000 * 1000,
};

volatile struct {
    uint32_t baudrate;
    uint32_t id0;
    uint32_t id1;
    uint32_t revision;
    uint32_t dev_ir;       /* device interrupt flags before / after clearing */
    uint32_t dev_ir_after;
    uint32_t spi_status;
    uint32_t mcan_ir;
    uint32_t nint_count;   /* falling edges on nINT */
    bool nint_low_before;  /* nINT asserted before clearing interrupts */
    bool nint_high_after;  /* nINT released after clearing interrupts */
    bool vsup_ok;          /* no undervoltage on VSUP */
    bool ok;
} probe;

static uint32_t ahb_read32(uint16_t addr)
{
    const uint8_t hdr[4] = {AHB_READ_OPCODE, addr >> 8, addr & 0xFF, 1};
    uint8_t d[4];
    cim_spi_select(&tcan_spi);
    cim_spi_write(&tcan_spi, hdr, sizeof hdr);
    cim_spi_read(&tcan_spi, d, sizeof d);
    cim_spi_deselect(&tcan_spi);
    return ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) | ((uint32_t)d[2] << 8) | d[3];
}

static void ahb_write32(uint16_t addr, uint32_t value)
{
    const uint8_t buf[8] = {AHB_WRITE_OPCODE, addr >> 8, addr & 0xFF, 1,
                            value >> 24, value >> 16, value >> 8, value};
    cim_spi_select(&tcan_spi);
    cim_spi_write(&tcan_spi, buf, sizeof buf);
    cim_spi_deselect(&tcan_spi);
}

static void on_nint(uint8_t pin, void *ctx)
{
    (void)pin;
    (*(volatile uint32_t *)ctx)++;
}

static void led(bool r, bool g, bool b)
{
    cim_gpio_put(CIM_LED_R_PIN, r != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_G_PIN, g != CIM_LED_ACTIVE_LOW);
    cim_gpio_put(CIM_LED_B_PIN, b != CIM_LED_ACTIVE_LOW);
}

int main(void)
{
    stdio_init_all();

    cim_gpio_init_out(CIM_LED_R_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_G_PIN, CIM_LED_ACTIVE_LOW);
    cim_gpio_init_out(CIM_LED_B_PIN, CIM_LED_ACTIVE_LOW);
    led(false, false, true);

    probe.baudrate = cim_spi_init(&tcan_spi);
    cim_gpio_init_in(CIM_TCAN_nINT_PIN, CIM_GPIO_PULL_UP);
    cim_gpio_irq_enable(CIM_TCAN_nINT_PIN, CIM_GPIO_EDGE_FALL, on_nint, (void *)&probe.nint_count);

    /* reset pulse (active high), then give the device time to start up */
    cim_gpio_init_out(CIM_TCAN_RST_PIN, false);
    cim_gpio_put(CIM_TCAN_RST_PIN, true);
    cim_delay_us(50);
    cim_gpio_put(CIM_TCAN_RST_PIN, false);
    cim_delay_ms(5);

    probe.id0 = ahb_read32(REG_SPI_DEVICE_ID0);
    probe.id1 = ahb_read32(REG_SPI_DEVICE_ID1);
    probe.revision = ahb_read32(REG_SPI_REVISION);

    probe.nint_low_before = !cim_gpio_get(CIM_TCAN_nINT_PIN);
    probe.dev_ir = ahb_read32(REG_DEV_IR);
    probe.spi_status = ahb_read32(REG_SPI_STATUS);
    probe.mcan_ir = ahb_read32(REG_MCAN_IR);
    /* clear all interrupt sources (write 1 to clear) */
    ahb_write32(REG_SPI_STATUS, 0xFFFFFFFF);
    ahb_write32(REG_MCAN_IR, 0xFFFFFFFF);
    ahb_write32(REG_DEV_IR, 0xFFFFFFFF);
    cim_delay_ms(1);
    probe.dev_ir_after = ahb_read32(REG_DEV_IR);
    probe.nint_high_after = cim_gpio_get(CIM_TCAN_nINT_PIN);

    probe.vsup_ok = !(probe.dev_ir_after & DEV_IR_UVSUP);
    probe.ok = probe.id0 == DEVICE_ID0_TCAN && (probe.dev_ir & DEV_IR_PWRON) && probe.nint_count > 0 &&
               (probe.nint_high_after || !probe.vsup_ok);
    led(!probe.ok || !probe.vsup_ok, probe.ok, false);

    for (;;) {
        printf("tcan_probe: %s%s id=0x%08lx 0x%08lx rev=0x%08lx dev_ir=0x%08lx->0x%08lx spi_status=0x%08lx mcan_ir=0x%08lx nINT edges=%lu low_before=%d high_after=%d spi=%lu Hz\n",
               probe.ok ? "OK" : "FAIL", probe.vsup_ok ? "" : " (no VSUP)", (unsigned long)probe.id0, (unsigned long)probe.id1,
               (unsigned long)probe.revision, (unsigned long)probe.dev_ir, (unsigned long)probe.dev_ir_after,
               (unsigned long)probe.spi_status, (unsigned long)probe.mcan_ir, (unsigned long)probe.nint_count, probe.nint_low_before,
               probe.nint_high_after, (unsigned long)probe.baudrate);
        sleep_ms(1000);
    }
}
