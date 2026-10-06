/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Persistent key/value configuration in flash.
 *
 * The configuration (node address, calibration values, ...) is stored as one
 * image in one of two flash sectors at the end of the flash. Every change
 * writes a complete new image to the other sector; the image with the highest
 * sequence number and a valid CRC wins. A power loss while writing therefore
 * keeps the previous configuration.
 *
 * Reads go directly to memory-mapped flash and need no initialisation, so the
 * bootloader can use cim_config_get() too.
 *
 * Writes erase a flash sector. Flash is rated for ~100k erase cycles per
 * sector, so write on configuration changes, not periodically.
 */

#ifndef CIM_CONFIG_H
#define CIM_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------- Keys ----------------------------------- */

/* 0x0001..0x7FFF: platform keys, 0x8000..0xFFFE: application keys */
#define CIM_CONFIG_KEY_ADDRESS   0x0001u /**< u8: node address on the CAN bus (ADR 0002) */
#define CIM_CONFIG_KEY_NAME      0x0002u /**< string: human-readable node name */
#define CIM_CONFIG_KEY_APP_FIRST 0x8000u /**< first key free for applications */

#define CIM_CONFIG_MAX_VALUE_LEN 255u

/* --------------------------------- Layout --------------------------------- */

/** Flash offset of the two configuration sectors (default: the last two sectors). */
#ifndef CIM_CONFIG_FLASH_OFFSET
#define CIM_CONFIG_FLASH_OFFSET (cim_flash_size() - 2u * CIM_FLASH_SECTOR_SIZE)
#endif

/* ---------------------------------- API ----------------------------------- */

typedef enum {
    CIM_CONFIG_OK = 0,
    CIM_CONFIG_ERR_NOT_FOUND = -1, /**< key not set */
    CIM_CONFIG_ERR_PARAM = -2,     /**< invalid key, NULL pointer or value too long */
    CIM_CONFIG_ERR_FULL = -3,      /**< image would exceed one sector */
    CIM_CONFIG_ERR_FLASH = -4,     /**< erase/program failed or read-back differs */
} cim_config_err_t;

/**
 * Copy the value of key into buf (at most buf_len bytes).
 * Returns the value length (which may exceed buf_len), or a negative cim_config_err_t.
 */
int cim_config_get(uint16_t key, void *buf, size_t buf_len);

/** Set key to value (len bytes, at most CIM_CONFIG_MAX_VALUE_LEN) and write it to flash. */
cim_config_err_t cim_config_set(uint16_t key, const void *value, size_t len);

/** Remove key and write the configuration to flash. Removing a missing key is OK. */
cim_config_err_t cim_config_delete(uint16_t key);

/** Erase both configuration sectors: all keys are gone. */
cim_config_err_t cim_config_erase_all(void);

/** Read a u8 value, or return def if the key is missing or has a different size. */
uint8_t cim_config_get_u8(uint16_t key, uint8_t def);

/** Read a u32 value (little-endian), or return def if the key is missing or has a different size. */
uint32_t cim_config_get_u32(uint16_t key, uint32_t def);

cim_config_err_t cim_config_set_u8(uint16_t key, uint8_t value);
cim_config_err_t cim_config_set_u32(uint16_t key, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* CIM_CONFIG_H */
