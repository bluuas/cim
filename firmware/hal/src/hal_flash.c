/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cim/hal.h"

#include "hardware/flash.h"
#include "pico/flash.h"

#define FLASH_TIMEOUT_MS 1000

typedef struct {
    uint32_t offset;
    const uint8_t *data; /* NULL: erase */
    uint32_t len;
} flash_op_t;

/* Runs with XIP disabled: interrupts off and the other core paused by flash_safe_execute(). */
static void flash_op(void *param)
{
    const flash_op_t *op = param;
    if (op->data == NULL) {
        flash_range_erase(op->offset, op->len);
    } else {
        flash_range_program(op->offset, op->data, op->len);
    }
}

/* align must be a power of two */
static bool in_range(uint32_t offset, uint32_t len, uint32_t align)
{
    return len > 0 && (offset & (align - 1)) == 0 && (len & (align - 1)) == 0 && offset <= PICO_FLASH_SIZE_BYTES &&
           len <= PICO_FLASH_SIZE_BYTES - offset;
}

uint32_t cim_flash_size(void)
{
    return PICO_FLASH_SIZE_BYTES;
}

const uint8_t *cim_flash_ptr(uint32_t offset)
{
    return (const uint8_t *)(XIP_BASE + offset);
}

bool cim_flash_erase(uint32_t offset, uint32_t len)
{
    if (!in_range(offset, len, CIM_FLASH_SECTOR_SIZE)) {
        return false;
    }
    flash_op_t op = {.offset = offset, .data = NULL, .len = len};
    return flash_safe_execute(flash_op, &op, FLASH_TIMEOUT_MS) == PICO_OK;
}

bool cim_flash_program(uint32_t offset, const uint8_t *data, uint32_t len)
{
    if (data == NULL || !in_range(offset, len, CIM_FLASH_PAGE_SIZE)) {
        return false;
    }
    flash_op_t op = {.offset = offset, .data = data, .len = len};
    return flash_safe_execute(flash_op, &op, FLASH_TIMEOUT_MS) == PICO_OK;
}
