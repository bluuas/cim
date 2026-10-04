/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * RAM-backed implementation of the HAL flash API for host tests, with NOR
 * semantics (erase sets 0xFF, program can only clear bits) and simulated
 * power loss.
 */

#include <string.h>

#include "flash_mock.h"

uint8_t flash_mock_mem[FLASH_MOCK_SIZE];
flash_mock_stats_t flash_mock_stats;

static int fail_countdown = -1; /* operations until power loss, -1: never */

void flash_mock_reset(void)
{
    memset(flash_mock_mem, 0xFF, sizeof flash_mock_mem);
    memset(&flash_mock_stats, 0, sizeof flash_mock_stats);
    fail_countdown = -1;
}

void flash_mock_power_loss_after(int ops)
{
    fail_countdown = ops;
}

/* Returns true if this operation is cut off by a power loss. */
static bool power_lost(void)
{
    if (fail_countdown < 0) {
        return false;
    }
    if (fail_countdown == 0) {
        fail_countdown = -1;
        return true;
    }
    fail_countdown--;
    return false;
}

uint32_t cim_flash_size(void)
{
    return FLASH_MOCK_SIZE;
}

const uint8_t *cim_flash_ptr(uint32_t offset)
{
    return flash_mock_mem + offset;
}

bool cim_flash_erase(uint32_t offset, uint32_t len)
{
    if (len == 0 || offset % CIM_FLASH_SECTOR_SIZE || len % CIM_FLASH_SECTOR_SIZE || offset + len > FLASH_MOCK_SIZE) {
        return false;
    }
    flash_mock_stats.erases++;
    if (power_lost()) {
        /* half erased, the rest still holds old data with a corrupted start */
        memset(flash_mock_mem + offset, 0xFF, len / 2);
        flash_mock_mem[offset + len / 2] ^= 0x5A;
        return false;
    }
    memset(flash_mock_mem + offset, 0xFF, len);
    return true;
}

bool cim_flash_program(uint32_t offset, const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0 || offset % CIM_FLASH_PAGE_SIZE || len % CIM_FLASH_PAGE_SIZE ||
        offset + len > FLASH_MOCK_SIZE) {
        return false;
    }
    flash_mock_stats.programs++;
    uint32_t n = power_lost() ? 8 : len; /* cut off inside the image header */
    for (uint32_t i = 0; i < n; i++) {
        flash_mock_mem[offset + i] &= data[i]; /* NOR: bits can only go from 1 to 0 */
    }
    return n == len;
}
