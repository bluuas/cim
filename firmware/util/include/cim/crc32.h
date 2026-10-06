/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CIM_CRC32_H
#define CIM_CRC32_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CRC-32 (IEEE 802.3, as used by zlib): reflected polynomial 0xEDB88320,
 * initial value and final XOR 0xFFFFFFFF.
 *
 * To compute over several buffers, pass the previous result as crc:
 *   crc = cim_crc32(0, a, len_a); crc = cim_crc32(crc, b, len_b);
 */
uint32_t cim_crc32(uint32_t crc, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CIM_CRC32_H */
