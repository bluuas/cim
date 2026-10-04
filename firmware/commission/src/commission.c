/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cim/commission.h"
#include "cim/config.h"

/* Return the next word of *s (NUL-terminated in place) and advance *s. */
static char *next_word(char **s)
{
    char *p = *s;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    if (*p == '\0') {
        *s = p;
        return NULL;
    }
    char *word = p;
    while (*p != '\0' && *p != ' ' && *p != '\t') {
        p++;
    }
    if (*p != '\0') {
        *p++ = '\0';
    }
    *s = p;
    return word;
}

/* Rest of the line without leading and trailing blanks. */
static char *rest_of_line(char *s)
{
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t')) {
        s[--n] = '\0';
    }
    return s;
}

static void uid_hex(char hex[17])
{
    uint8_t uid[8];
    cim_commission_get_uid(uid);
    for (int i = 0; i < 8; i++) {
        snprintf(&hex[2 * i], 3, "%02X", uid[i]);
    }
}

static void get_name(char name[CIM_COMMISSION_NAME_MAX + 1])
{
    int n = cim_config_get(CIM_CONFIG_KEY_NAME, name, CIM_COMMISSION_NAME_MAX);
    name[n > 0 ? (n < (int)CIM_COMMISSION_NAME_MAX ? n : (int)CIM_COMMISSION_NAME_MAX) : 0] = '\0';
}

static void cmd_info(char *out, size_t out_len)
{
    char hex[17], name[CIM_COMMISSION_NAME_MAX + 1];
    uid_hex(hex);
    get_name(name);
    snprintf(out, out_len, "ok address=%u name=%s uid=%s", cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0), name, hex);
}

static void cmd_set_address(char *arg, char *out, size_t out_len)
{
    char *end;
    unsigned long v = arg ? strtoul(arg, &end, 0) : 0;
    if (arg == NULL || *end != '\0' || v < CIM_COMMISSION_ADDRESS_MIN || v > CIM_COMMISSION_ADDRESS_MAX) {
        snprintf(out, out_len, "error address must be %u..%u", CIM_COMMISSION_ADDRESS_MIN, CIM_COMMISSION_ADDRESS_MAX);
        return;
    }
    if (cim_config_set_u8(CIM_CONFIG_KEY_ADDRESS, (uint8_t)v) != CIM_CONFIG_OK) {
        snprintf(out, out_len, "error flash write failed");
        return;
    }
    snprintf(out, out_len, "ok address=%u", cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0));
}

static void cmd_set_name(char *arg, char *out, size_t out_len)
{
    size_t n = strlen(arg);
    if (n == 0 || n > CIM_COMMISSION_NAME_MAX) {
        snprintf(out, out_len, "error name must be 1..%u characters", CIM_COMMISSION_NAME_MAX);
        return;
    }
    for (size_t i = 0; i < n; i++) {
        if (!isprint((unsigned char)arg[i])) {
            snprintf(out, out_len, "error name must be printable");
            return;
        }
    }
    if (cim_config_set(CIM_CONFIG_KEY_NAME, arg, n) != CIM_CONFIG_OK) {
        snprintf(out, out_len, "error flash write failed");
        return;
    }
    char name[CIM_COMMISSION_NAME_MAX + 1];
    get_name(name);
    snprintf(out, out_len, "ok name=%s", name);
}

void cim_commission_execute(const char *line, char *out, size_t out_len)
{
    char buf[96];
    snprintf(buf, sizeof buf, "%s", line);
    char *s = buf;
    char *cmd = next_word(&s);

    if (cmd == NULL) {
        out[0] = '\0';
    } else if (strcmp(cmd, "help") == 0) {
        snprintf(out, out_len,
                 "ok commands: info, uid, get address, set address <%u..%u>, get name, set name <text>, reboot [bootsel]",
                 CIM_COMMISSION_ADDRESS_MIN, CIM_COMMISSION_ADDRESS_MAX);
    } else if (strcmp(cmd, "info") == 0) {
        cmd_info(out, out_len);
    } else if (strcmp(cmd, "uid") == 0) {
        char hex[17];
        uid_hex(hex);
        snprintf(out, out_len, "ok uid=%s", hex);
    } else if (strcmp(cmd, "get") == 0 || strcmp(cmd, "set") == 0) {
        char *what = next_word(&s);
        bool set = cmd[0] == 's';
        if (what != NULL && strcmp(what, "address") == 0) {
            if (set) {
                cmd_set_address(next_word(&s), out, out_len);
            } else {
                snprintf(out, out_len, "ok address=%u", cim_config_get_u8(CIM_CONFIG_KEY_ADDRESS, 0));
            }
        } else if (what != NULL && strcmp(what, "name") == 0) {
            if (set) {
                cmd_set_name(rest_of_line(s), out, out_len);
            } else {
                char name[CIM_COMMISSION_NAME_MAX + 1];
                get_name(name);
                snprintf(out, out_len, "ok name=%s", name);
            }
        } else {
            snprintf(out, out_len, "error unknown setting, use address or name");
        }
    } else if (strcmp(cmd, "reboot") == 0) {
        char *mode = next_word(&s);
        if (mode != NULL && strcmp(mode, "bootsel") != 0) {
            snprintf(out, out_len, "error usage: reboot [bootsel]");
            return;
        }
        cim_commission_reboot(mode != NULL);
    } else {
        snprintf(out, out_len, "error unknown command '%s', try help", cmd);
    }
}

void cim_commission_poll(void)
{
    static char line[96];
    static size_t len;
    static bool overflow;

    for (;;) {
        int c = cim_commission_getchar();
        if (c < 0) {
            return;
        }
        if (c == '\r' || c == '\n') {
            if (overflow) {
                printf("error line too long\n");
            } else if (len > 0) {
                line[len] = '\0';
                char out[160];
                cim_commission_execute(line, out, sizeof out);
                if (out[0] != '\0') {
                    printf("%s\n", out);
                }
            }
            len = 0;
            overflow = false;
        } else if (len < sizeof line - 1) {
            line[len++] = (char)c;
        } else {
            overflow = true;
        }
    }
}
