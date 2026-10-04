/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cim/crc32.h"

/* Bitwise implementation: small, no table. Fast enough for config and firmware blocks. */
uint32_t cim_crc32(uint32_t crc, const void *data, size_t len)
{
    const uint8_t *p = data;
    crc = ~crc;
    while (len--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
        }
    }
    return ~crc;
}
