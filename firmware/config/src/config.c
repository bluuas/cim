/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Image format (little-endian), one image per sector:
 *
 *   offset 0   u32 magic "CIMC"
 *          4   u16 format version (1)
 *          6   u16 payload length in bytes
 *          8   u32 sequence number, incremented with every write
 *         12   u32 CRC-32 over bytes 0..11 and the payload
 *         16   payload: records of { u16 key, u8 length, u8 value[length] }
 *
 * The rest of the sector is erased (0xFF).
 */

#include <string.h>

#include "cim/config.h"
#include "cim/crc32.h"
#include "cim/hal.h"

#define MAGIC          0x434D4943u /* "CIMC" */
#define FORMAT_VERSION 1u
#define HEADER_SIZE    16u
#define RECORD_HEADER  3u
#define MAX_PAYLOAD    (CIM_FLASH_SECTOR_SIZE - HEADER_SIZE)
#define KEY_INVALID    0xFFFFu

typedef struct {
    const uint8_t *payload;
    uint16_t len;
    uint32_t seq;
    int sector; /* 0 or 1, -1: no valid image */
} image_t;

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)(p[0] | p[1] << 8);
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void wr32(uint8_t *p, uint32_t v)
{
    for (int i = 0; i < 4; i++) {
        p[i] = (uint8_t)(v >> (8 * i));
    }
}

static uint32_t sector_offset(int sector)
{
    return CIM_CONFIG_FLASH_OFFSET + (uint32_t)sector * CIM_FLASH_SECTOR_SIZE;
}

static uint32_t image_crc(const uint8_t *header, const uint8_t *payload, uint16_t len)
{
    return cim_crc32(cim_crc32(0, header, 12), payload, len);
}

static bool image_valid(const uint8_t *s, uint16_t *len, uint32_t *seq)
{
    if (rd32(s) != MAGIC || rd16(s + 4) != FORMAT_VERSION) {
        return false;
    }
    uint16_t l = rd16(s + 6);
    if (l > MAX_PAYLOAD || image_crc(s, s + HEADER_SIZE, l) != rd32(s + 12)) {
        return false;
    }
    *len = l;
    *seq = rd32(s + 8);
    return true;
}

/* Find the newest valid image. Sequence numbers are compared with wrap-around. */
static image_t current_image(void)
{
    image_t img = {.payload = NULL, .len = 0, .seq = 0, .sector = -1};
    for (int i = 0; i < 2; i++) {
        const uint8_t *s = cim_flash_ptr(sector_offset(i));
        uint16_t len;
        uint32_t seq;
        if (image_valid(s, &len, &seq) && (img.sector < 0 || (int32_t)(seq - img.seq) > 0)) {
            img = (image_t){.payload = s + HEADER_SIZE, .len = len, .seq = seq, .sector = i};
        }
    }
    return img;
}

/* Find key in a payload. Returns a pointer to its record, or NULL. */
static const uint8_t *find_record(const uint8_t *payload, uint16_t len, uint16_t key)
{
    uint16_t pos = 0;
    while (pos + RECORD_HEADER <= len) {
        const uint8_t *r = payload + pos;
        uint16_t next = (uint16_t)(pos + RECORD_HEADER + r[2]);
        if (next > len) {
            break; /* malformed: cannot happen with a valid CRC */
        }
        if (rd16(r) == key) {
            return r;
        }
        pos = next;
    }
    return NULL;
}

int cim_config_get(uint16_t key, void *buf, size_t buf_len)
{
    if (key == KEY_INVALID || (buf == NULL && buf_len > 0)) {
        return CIM_CONFIG_ERR_PARAM;
    }
    image_t img = current_image();
    const uint8_t *r = img.sector < 0 ? NULL : find_record(img.payload, img.len, key);
    if (r == NULL) {
        return CIM_CONFIG_ERR_NOT_FOUND;
    }
    uint8_t len = r[2];
    if (buf_len > 0) {
        memcpy(buf, r + RECORD_HEADER, len < buf_len ? len : buf_len);
    }
    return len;
}

/* Sector-sized buffer for building a new image. */
static uint8_t image_buf[CIM_FLASH_SECTOR_SIZE];

/*
 * Write a new image with all records of the current image except key, plus
 * key = value if value is not NULL. The new image goes to the other sector,
 * so the current image stays intact until the new one is complete.
 */
static cim_config_err_t write_image(uint16_t key, const void *value, uint8_t len)
{
    image_t cur = current_image();
    uint8_t *payload = image_buf + HEADER_SIZE;
    uint16_t out = 0;

    uint16_t pos = 0;
    while (cur.sector >= 0 && pos + RECORD_HEADER <= cur.len) {
        const uint8_t *r = cur.payload + pos;
        uint16_t rec_len = (uint16_t)(RECORD_HEADER + r[2]);
        if (pos + rec_len > cur.len) {
            break;
        }
        if (rd16(r) != key) {
            memcpy(payload + out, r, rec_len); /* fits: the current image fit as well */
            out = (uint16_t)(out + rec_len);
        }
        pos = (uint16_t)(pos + rec_len);
    }
    if (value != NULL) {
        if (out + RECORD_HEADER + len > MAX_PAYLOAD) {
            return CIM_CONFIG_ERR_FULL;
        }
        wr16(payload + out, key);
        payload[out + 2] = len;
        memcpy(payload + out + RECORD_HEADER, value, len);
        out = (uint16_t)(out + RECORD_HEADER + len);
    }

    int target = cur.sector < 0 ? 0 : 1 - cur.sector;
    wr32(image_buf, MAGIC);
    wr16(image_buf + 4, FORMAT_VERSION);
    wr16(image_buf + 6, out);
    wr32(image_buf + 8, cur.sector < 0 ? 1 : cur.seq + 1);
    wr32(image_buf + 12, image_crc(image_buf, payload, out));

    /* program whole pages; the padding stays erased */
    uint32_t size = HEADER_SIZE + out;
    size = (size + CIM_FLASH_PAGE_SIZE - 1) / CIM_FLASH_PAGE_SIZE * CIM_FLASH_PAGE_SIZE;
    memset(image_buf + HEADER_SIZE + out, 0xFF, size - HEADER_SIZE - out);

    uint32_t offset = sector_offset(target);
    if (!cim_flash_erase(offset, CIM_FLASH_SECTOR_SIZE) || !cim_flash_program(offset, image_buf, size) ||
        memcmp(cim_flash_ptr(offset), image_buf, size) != 0) {
        return CIM_CONFIG_ERR_FLASH;
    }
    return CIM_CONFIG_OK;
}

cim_config_err_t cim_config_set(uint16_t key, const void *value, size_t len)
{
    if (key == KEY_INVALID || (value == NULL && len > 0) || len > CIM_CONFIG_MAX_VALUE_LEN) {
        return CIM_CONFIG_ERR_PARAM;
    }
    /* skip the flash write if nothing changes */
    uint8_t old[CIM_CONFIG_MAX_VALUE_LEN];
    if (cim_config_get(key, old, sizeof old) == (int)len && (len == 0 || memcmp(old, value, len) == 0)) {
        return CIM_CONFIG_OK;
    }
    static const uint8_t empty;
    return write_image(key, len > 0 ? value : &empty, (uint8_t)len);
}

cim_config_err_t cim_config_delete(uint16_t key)
{
    if (key == KEY_INVALID) {
        return CIM_CONFIG_ERR_PARAM;
    }
    if (cim_config_get(key, NULL, 0) == CIM_CONFIG_ERR_NOT_FOUND) {
        return CIM_CONFIG_OK;
    }
    return write_image(key, NULL, 0);
}

cim_config_err_t cim_config_erase_all(void)
{
    return cim_flash_erase(CIM_CONFIG_FLASH_OFFSET, 2 * CIM_FLASH_SECTOR_SIZE) ? CIM_CONFIG_OK : CIM_CONFIG_ERR_FLASH;
}

uint8_t cim_config_get_u8(uint16_t key, uint8_t def)
{
    uint8_t v;
    return cim_config_get(key, &v, sizeof v) == (int)sizeof v ? v : def;
}

uint32_t cim_config_get_u32(uint16_t key, uint32_t def)
{
    uint8_t v[4];
    return cim_config_get(key, v, sizeof v) == (int)sizeof v ? rd32(v) : def;
}

cim_config_err_t cim_config_set_u8(uint16_t key, uint8_t value)
{
    return cim_config_set(key, &value, sizeof value);
}

cim_config_err_t cim_config_set_u32(uint16_t key, uint32_t value)
{
    uint8_t v[4];
    wr32(v, value);
    return cim_config_set(key, v, sizeof v);
}
