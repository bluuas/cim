/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Checks the HAL and the TCAN4x5x core driver: reset via GPIO, read the device
 * ID over SPI, and count nINT interrupts (the device signals power-on after reset).
 * LED: green = ID read correctly and power-on interrupt seen on nINT,
 *      yellow = OK but VSUP (12 V) missing (UVSUP, keeps nINT asserted),
 *      red = failure.
 * The result is also printed over USB CDC and kept in `probe` for a debugger.
 */

#include <stdio.h>

#include "pico/stdlib.h"

#include "TCAN4550.h"
#include "cim/hal.h"

#define DEVICE_ID0_TCAN 0x4E414354u /* "TCAN", LSB first */

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
    uint32_t mode;         /* TCAN4x5x_Device_Mode_Enum */
    uint32_t nint_count;   /* falling edges on nINT */
    bool nint_low_before;  /* nINT asserted before clearing interrupts */
    bool nint_high_after;  /* nINT released after clearing interrupts */
    bool vsup_ok;          /* no undervoltage on VSUP */
    bool ok;
} probe;

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

    probe.baudrate = TCAN4x5x_SPI_Init(&tcan_spi);
    cim_gpio_init_in(CIM_TCAN_nINT_PIN, CIM_GPIO_PULL_UP);
    cim_gpio_irq_enable(CIM_TCAN_nINT_PIN, CIM_GPIO_EDGE_FALL, on_nint, (void *)&probe.nint_count);

    /* reset pulse (active high), then give the device time to start up */
    cim_gpio_init_out(CIM_TCAN_RST_PIN, false);
    cim_gpio_put(CIM_TCAN_RST_PIN, true);
    cim_delay_us(50);
    cim_gpio_put(CIM_TCAN_RST_PIN, false);
    cim_delay_ms(5);

    probe.id0 = AHB_READ_32(REG_SPI_DEVICE_ID0);
    probe.id1 = AHB_READ_32(REG_SPI_DEVICE_ID1);
    probe.revision = AHB_READ_32(REG_SPI_REVISION);
    probe.mode = TCAN4x5x_Device_ReadMode();

    TCAN4x5x_Device_Interrupts ir;
    probe.nint_low_before = !cim_gpio_get(CIM_TCAN_nINT_PIN);
    TCAN4x5x_Device_ReadInterrupts(&ir);
    probe.dev_ir = ir.word;

    TCAN4x5x_Device_ClearSPIERR();
    TCAN4x5x_MCAN_ClearInterruptsAll();
    TCAN4x5x_Device_ClearInterruptsAll();
    cim_delay_ms(1);

    TCAN4x5x_Device_ReadInterrupts(&ir);
    probe.dev_ir_after = ir.word;
    probe.nint_high_after = cim_gpio_get(CIM_TCAN_nINT_PIN);

    TCAN4x5x_Device_Interrupts before = {.word = probe.dev_ir};
    probe.vsup_ok = !ir.UVSUP;
    probe.ok = probe.id0 == DEVICE_ID0_TCAN && before.PWRON && probe.nint_count > 0 &&
               (probe.nint_high_after || !probe.vsup_ok);
    led(!probe.ok || !probe.vsup_ok, probe.ok, false);

    for (;;) {
        printf("tcan_probe: %s%s id=0x%08lx 0x%08lx rev=0x%08lx mode=%lu dev_ir=0x%08lx->0x%08lx "
               "nINT edges=%lu low_before=%d high_after=%d spi=%lu Hz\n",
               probe.ok ? "OK" : "FAIL", probe.vsup_ok ? "" : " (no VSUP)", (unsigned long)probe.id0,
               (unsigned long)probe.id1, (unsigned long)probe.revision, (unsigned long)probe.mode,
               (unsigned long)probe.dev_ir, (unsigned long)probe.dev_ir_after, (unsigned long)probe.nint_count,
               probe.nint_low_before, probe.nint_high_after, (unsigned long)probe.baudrate);
        sleep_ms(1000);
    }
}
