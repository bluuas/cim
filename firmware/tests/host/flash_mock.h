/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FLASH_MOCK_H
#define FLASH_MOCK_H

#include <stdint.h>

#include "cim/hal.h"

#define FLASH_MOCK_SIZE (64u * 1024u)

typedef struct {
    unsigned erases;
    unsigned programs;
} flash_mock_stats_t;

extern uint8_t flash_mock_mem[FLASH_MOCK_SIZE];
extern flash_mock_stats_t flash_mock_stats;

/** Erase the whole mock flash and reset statistics and failure injection. */
void flash_mock_reset(void);

/**
 * Simulate a power loss: the given number of erase/program operations succeed,
 * the next one is cut off halfway and fails.
 */
void flash_mock_power_loss_after(int ops);

#endif /* FLASH_MOCK_H */
