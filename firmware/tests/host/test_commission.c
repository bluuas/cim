/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Host tests for the commissioning commands (cim/commission.h).
 */

#include <string.h>

#include "cim/commission.h"
#include "cim/config.h"
#include "flash_mock.h"
#include "test.h"

static int reboot_mode = -1;

int cim_commission_getchar(void)
{
    return -1;
}

void cim_commission_get_uid(uint8_t uid[8])
{
    static const uint8_t id[8] = {0xE6, 0x61, 0x41, 0x03, 0xE7, 0x45, 0x2D, 0x2F};
    memcpy(uid, id, 8);
}

void cim_commission_reboot(int bootsel)
{
    reboot_mode = bootsel;
}

static char out[160];

static const char *run(const char *line)
{
    cim_commission_execute(line, out, sizeof out);
    return out;
}

#define CHECK_OUT(line, expected) CHECK(strcmp(run(line), expected) == 0 || (printf("  got: '%s'\n", out), 0))

static void test_info_unconfigured(void)
{
    flash_mock_reset();
    CHECK_OUT("info", "ok address=0 name= uid=E6614103E7452D2F");
    CHECK_OUT("uid", "ok uid=E6614103E7452D2F");
    CHECK_OUT("get address", "ok address=0");
    CHECK_OUT("get name", "ok name=");
}

static void test_set_address(void)
{
    flash_mock_reset();
    CHECK_OUT("set address 5", "ok address=5");
    CHECK_OUT("  set   address   0x10  ", "ok address=16");
    CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 16);
    CHECK_OUT("set address 0", "error address must be 1..239");
    CHECK_OUT("set address 240", "error address must be 1..239");
    CHECK_OUT("set address 5x", "error address must be 1..239");
    CHECK_OUT("set address", "error address must be 1..239");
    CHECK_EQ(cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), 16); /* unchanged after errors */
}

static void test_set_name(void)
{
    flash_mock_reset();
    CHECK_OUT("set name pedalbox front", "ok name=pedalbox front");
    CHECK_OUT("get name", "ok name=pedalbox front");
    CHECK_OUT("set name", "error name must be 1..32 characters");
    CHECK_OUT("set name 123456789012345678901234567890123", "error name must be 1..32 characters");
    CHECK_OUT("set address 7", "ok address=7");
    CHECK_OUT("info", "ok address=7 name=pedalbox front uid=E6614103E7452D2F");
}

static void test_errors_and_misc(void)
{
    flash_mock_reset();
    CHECK_OUT("", "");
    CHECK_OUT("frobnicate", "error unknown command 'frobnicate', try help");
    CHECK_OUT("get colour", "error unknown setting, use address or name");
    CHECK(strncmp(run("help"), "ok commands:", 12) == 0);
    CHECK_OUT("reboot now", "error usage: reboot [bootsel]");
    reboot_mode = -1;
    run("reboot");
    CHECK_EQ(reboot_mode, 0);
    run("reboot bootsel");
    CHECK_EQ(reboot_mode, 1);
}

int main(void)
{
    RUN(test_info_unconfigured);
    RUN(test_set_address);
    RUN(test_set_name);
    RUN(test_errors_and_misc);
    return test_summary();
}
