/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Host tests for the configuration store (cim/config.h) and CRC-32.
 */

#include <stdio.h>
#include <string.h>

#include "cim/config.h"
#include "cim/crc32.h"
#include "flash_mock.h"
#include "test.h"

#define CONFIG_SECTOR(i) (CIM_CONFIG_FLASH_OFFSET + (i) * CIM_FLASH_SECTOR_SIZE)

static void test_crc32(void)
{
    CHECK_EQ(cim_crc32(0, "123456789", 9), 0xCBF43926u); /* standard check value */
    uint32_t crc = cim_crc32(0, "1234", 4);
    CHECK_EQ(cim_crc32(crc, "56789", 5), 0xCBF43926u);   /* incremental */
    CHECK_EQ(cim_crc32(0, "", 0), 0u);
}

static void test_empty(void)
{
    flash_mock_reset();
    uint8_t buf[4];
    CHECK_EQ(cim_config_get(CIM_CONFIG_KEY_ADDRESS, buf, sizeof buf), CIM_CONFIG_ERR_NOT_FOUND);
    CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 0);
    CHECK_EQ(cim_config_get_u32(0x8000, 1234), 1234u);
}

static void test_set_get(void)
{
    flash_mock_reset();
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 5), CIM_CONFIG_OK);
    CHECK_EQ(cim_config_set_u32(0x8000, 0xDEADBEEF), CIM_CONFIG_OK);
    CHECK_EQ(cim_config_set(CIM_CONFIG_KEY_NAME, "pedalbox", 8), CIM_CONFIG_OK);

    CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 5);
    CHECK_EQ(cim_config_get_u32(0x8000, 0), 0xDEADBEEFu);
    char name[16] = {0};
    CHECK_EQ(cim_config_get(CIM_CONFIG_KEY_NAME, name, sizeof name), 8);
    CHECK(strcmp(name, "pedalbox") == 0);

    /* short buffer: copy is truncated, full length is returned */
    char shortbuf[3];
    CHECK_EQ(cim_config_get(CIM_CONFIG_KEY_NAME, shortbuf, sizeof shortbuf), 8);
    CHECK(memcmp(shortbuf, "ped", 3) == 0);

    /* overwrite keeps other keys */
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 7), CIM_CONFIG_OK);
    CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 7);
    CHECK_EQ(cim_config_get_u32(0x8000, 0), 0xDEADBEEFu);

    /* wrong size falls back to the default */
    CHECK_EQ(cim_config_get_u32(CIM_CONFIG_KEY_ADDRESS, 99), 99u);

    /* zero-length values are allowed */
    CHECK_EQ(cim_config_set(0x8001, NULL, 0), CIM_CONFIG_OK);
    CHECK_EQ(cim_config_get(0x8001, NULL, 0), 0);
}

static void test_alternating_sectors(void)
{
    flash_mock_reset();
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 1), CIM_CONFIG_OK);
    CHECK(memcmp(&flash_mock_mem[CONFIG_SECTOR(0)], "CIMC", 4) == 0);
    CHECK_EQ(flash_mock_mem[CONFIG_SECTOR(1)], 0xFF);
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 2), CIM_CONFIG_OK);
    CHECK(memcmp(&flash_mock_mem[CONFIG_SECTOR(1)], "CIMC", 4) == 0);

    for (unsigned i = 0; i < 1000; i++) {
        CHECK_EQ(cim_config_set_u32(0x8000, i), CIM_CONFIG_OK);
    }
    CHECK_EQ(cim_config_get_u32(0x8000, 0), 999u);
    CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 2);
}

static void test_unchanged_value_not_written(void)
{
    flash_mock_reset();
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 5), CIM_CONFIG_OK);
    unsigned erases = flash_mock_stats.erases;
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 5), CIM_CONFIG_OK);
    CHECK_EQ(flash_mock_stats.erases, erases);
}

static void test_delete(void)
{
    flash_mock_reset();
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 5), CIM_CONFIG_OK);
    CHECK_EQ(cim_config_set_u8(0x8000, 1), CIM_CONFIG_OK);
    CHECK_EQ(cim_config_delete(CIM_CONFIG_KEY_ADDRESS), CIM_CONFIG_OK);
    CHECK_EQ(cim_config_get(CIM_CONFIG_KEY_ADDRESS, NULL, 0), CIM_CONFIG_ERR_NOT_FOUND);
    CHECK_EQ(cim_config_get_u8(0x8000, 0), 1);
    CHECK_EQ(cim_config_delete(0x1234), CIM_CONFIG_OK); /* missing key */

    CHECK_EQ(cim_config_erase_all(), CIM_CONFIG_OK);
    CHECK_EQ(cim_config_get(0x8000, NULL, 0), CIM_CONFIG_ERR_NOT_FOUND);
}

static void test_params(void)
{
    flash_mock_reset();
    uint8_t big[CIM_CONFIG_MAX_VALUE_LEN + 1] = {0};
    CHECK_EQ(cim_config_set(0x8000, big, sizeof big), CIM_CONFIG_ERR_PARAM);
    CHECK_EQ(cim_config_set(0xFFFF, big, 1), CIM_CONFIG_ERR_PARAM);
    CHECK_EQ(cim_config_set(0x8000, NULL, 1), CIM_CONFIG_ERR_PARAM);
    CHECK_EQ(cim_config_get(0x8000, NULL, 1), CIM_CONFIG_ERR_PARAM);
}

static void test_full(void)
{
    flash_mock_reset();
    uint8_t value[CIM_CONFIG_MAX_VALUE_LEN];
    memset(value, 0xAB, sizeof value);
    uint16_t key = 0x8000;
    cim_config_err_t err;
    while ((err = cim_config_set(key, value, sizeof value)) == CIM_CONFIG_OK) {
        key++;
    }
    CHECK_EQ(err, CIM_CONFIG_ERR_FULL);
    CHECK_EQ(key - 0x8000, 15); /* 15 * 258 bytes fit into 4096 - 16 */
    /* existing values are still there, and small values can be updated */
    CHECK_EQ(cim_config_get(0x8000, NULL, 0), (int)sizeof value);
    CHECK_EQ(cim_config_set(0x8000, value, 10), CIM_CONFIG_OK);
}

static void test_power_loss(void)
{
    for (int ops = 0; ops < 2; ops++) { /* 0: during erase, 1: during program */
        flash_mock_reset();
        CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 5), CIM_CONFIG_OK);
        CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 6), CIM_CONFIG_OK);

        flash_mock_power_loss_after(ops);
        CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 7), CIM_CONFIG_ERR_FLASH);
        CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 6); /* previous value survives */

        CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 8), CIM_CONFIG_OK); /* recovers */
        CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 8);
    }
}

static void test_corrupt_newest_falls_back(void)
{
    flash_mock_reset();
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 5), CIM_CONFIG_OK); /* sector 0 */
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 6), CIM_CONFIG_OK); /* sector 1 */
    flash_mock_mem[CONFIG_SECTOR(1) + 16 + 3] ^= 0x01; /* flip a bit in the value */
    CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 5);
}

static void test_sequence_wraparound(void)
{
    flash_mock_reset();
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 5), CIM_CONFIG_OK);
    /* pretend sector 0 holds sequence 0xFFFFFFFF: rewrite header and CRC */
    uint8_t *s = &flash_mock_mem[CONFIG_SECTOR(0)];
    s[8] = s[9] = s[10] = s[11] = 0xFF;
    uint16_t len = (uint16_t)(s[6] | s[7] << 8);
    uint32_t crc = cim_crc32(cim_crc32(0, s, 12), s + 16, len);
    for (int i = 0; i < 4; i++) {
        s[12 + i] = (uint8_t)(crc >> (8 * i));
    }
    CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 5);
    CHECK_EQ(cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, 6), CIM_CONFIG_OK); /* sequence 0 in sector 1 */
    CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 6);              /* 0 is newer than 0xFFFFFFFF */
}

int main(void)
{
    RUN(test_crc32);
    RUN(test_empty);
    RUN(test_set_get);
    RUN(test_alternating_sectors);
    RUN(test_unchanged_value_not_written);
    RUN(test_delete);
    RUN(test_params);
    RUN(test_full);
    RUN(test_power_loss);
    RUN(test_corrupt_newest_falls_back);
    RUN(test_sequence_wraparound);
    return test_summary();
}
