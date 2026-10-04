/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Implementation of TI's TCAN4x5x SPI abstraction (TCAN4x5x_SPI.h) on the
 * CIM HAL, replacing TI's MSP430 implementation (TCAN455x Driver Library
 * Demo 1.2.2). Function documentation follows TI's original. Original header below.
 */
/*
 * TCAN4x5x_SPI.c
 * Description: This file is responsible for abstracting the lower-level microcontroller SPI read and write functions
 *
 *
 *
 * Copyright (c) 2019 Texas Instruments Incorporated.  All rights reserved.
 * Software License Agreement
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <stddef.h>

#include "TCAN4x5x_SPI.h"

/*
 * Each AHB access is one SPI transaction under a single chip select:
 * 4 header bytes (opcode, 16 bit address, word count), then 4 bytes per word,
 * MSB first.
 */

static const cim_spi_dev_t *spi_dev;

uint32_t TCAN4x5x_SPI_Init(const cim_spi_dev_t *dev)
{
    spi_dev = dev;
    return cim_spi_init(dev);
}

static void send_header(uint8_t opcode, uint16_t address, uint8_t words)
{
    const uint8_t hdr[4] = {opcode, (uint8_t)(address >> 8), (uint8_t)address, words};
    assert(spi_dev != NULL);
    cim_spi_select(spi_dev);
    cim_spi_write(spi_dev, hdr, sizeof hdr);
}

/**
 * @brief Single word write
 *
 * @param address A 16-bit address of the destination register
 * @param data A 32-bit word of data to write to the destination register
 */
void AHB_WRITE_32(uint16_t address, uint32_t data)
{
    AHB_WRITE_BURST_START(address, 1);
    AHB_WRITE_BURST_WRITE(data);
    AHB_WRITE_BURST_END();
}

/**
 * @brief Single word read
 *
 * @param address A 16-bit address of the source register
 * @return 32-bit word of data from the source register
 */
uint32_t AHB_READ_32(uint16_t address)
{
    AHB_READ_BURST_START(address, 1);
    uint32_t data = AHB_READ_BURST_READ();
    AHB_READ_BURST_END();
    return data;
}

/**
 * @brief Burst write start: pulls nCS low and sends the header
 *
 * @param address A 16-bit address of the destination register
 * @param words The number of 4-byte words that will be transferred. 0 = 256 words
 */
void AHB_WRITE_BURST_START(uint16_t address, uint8_t words)
{
    send_header(AHB_WRITE_OPCODE, address, words);
}

/**
 * @brief Burst write of a single word
 *
 * @param data A 32-bit word of data to write
 */
void AHB_WRITE_BURST_WRITE(uint32_t data)
{
    const uint8_t buf[4] = {(uint8_t)(data >> 24), (uint8_t)(data >> 16), (uint8_t)(data >> 8), (uint8_t)data};
    cim_spi_write(spi_dev, buf, sizeof buf);
}

/**
 * @brief Burst write end: releases nCS
 */
void AHB_WRITE_BURST_END(void)
{
    cim_spi_deselect(spi_dev);
}

/**
 * @brief Burst read start: pulls nCS low and sends the header
 *
 * @param address A 16-bit start address to begin the burst read
 * @param words The number of 4-byte words that will be transferred. 0 = 256 words
 */
void AHB_READ_BURST_START(uint16_t address, uint8_t words)
{
    send_header(AHB_READ_OPCODE, address, words);
}

/**
 * @brief Burst read of a single word
 *
 * @return 32-bit word of data
 */
uint32_t AHB_READ_BURST_READ(void)
{
    uint8_t buf[4];
    cim_spi_read(spi_dev, buf, sizeof buf);
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
}

/**
 * @brief Burst read end: releases nCS
 */
void AHB_READ_BURST_END(void)
{
    cim_spi_deselect(spi_dev);
}
