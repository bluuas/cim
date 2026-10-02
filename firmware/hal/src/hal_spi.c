/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cim/hal.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"

static spi_inst_t *spi_inst(const cim_spi_dev_t *dev)
{
    return dev->spi == 0 ? spi0 : spi1;
}

uint32_t cim_spi_init(const cim_spi_dev_t *dev)
{
    spi_inst_t *spi = spi_inst(dev);
    /* spi_init() resets the format, so set it afterwards */
    uint32_t baud = spi_init(spi, dev->baudrate);
    spi_set_format(spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(dev->sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(dev->mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(dev->miso_pin, GPIO_FUNC_SPI);

    cim_gpio_init_out(dev->cs_pin, true);
    return baud;
}

void cim_spi_select(const cim_spi_dev_t *dev)
{
    gpio_put(dev->cs_pin, false);
}

void cim_spi_deselect(const cim_spi_dev_t *dev)
{
    gpio_put(dev->cs_pin, true);
}

void cim_spi_write(const cim_spi_dev_t *dev, const uint8_t *tx, size_t len)
{
    spi_write_blocking(spi_inst(dev), tx, len);
}

void cim_spi_read(const cim_spi_dev_t *dev, uint8_t *rx, size_t len)
{
    spi_read_blocking(spi_inst(dev), 0x00, rx, len);
}

void cim_spi_transfer(const cim_spi_dev_t *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    spi_write_read_blocking(spi_inst(dev), tx, rx, len);
}
