/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Minimal hardware abstraction used by CIM drivers.
 *
 * Only what the drivers need: SPI with manual chip select, GPIO,
 * GPIO edge interrupts with a context pointer, delays and flash access.
 * The header has no Pico SDK types so drivers can be unit-tested on the host
 * against a mock implementation.
 */

#ifndef CIM_HAL_H
#define CIM_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------- SPI ----------------------------------- */

/** SPI device: bus, pins and settings. Mode 0 (CPOL=0, CPHA=0), 8 bit, MSB first. */
typedef struct {
    uint8_t spi;      /**< SPI instance (0 or 1) */
    uint8_t sck_pin;
    uint8_t mosi_pin; /**< MCU TX */
    uint8_t miso_pin; /**< MCU RX */
    uint8_t cs_pin;   /**< chip select, active low, driven as GPIO */
    uint32_t baudrate;
} cim_spi_dev_t;

/** Initialise the SPI instance and pins. Returns the actual baudrate set. */
uint32_t cim_spi_init(const cim_spi_dev_t *dev);

/** Pull chip select low. A transaction may span several write/read calls. */
void cim_spi_select(const cim_spi_dev_t *dev);

/** Release chip select (high). */
void cim_spi_deselect(const cim_spi_dev_t *dev);

/** Write len bytes, discarding received data. */
void cim_spi_write(const cim_spi_dev_t *dev, const uint8_t *tx, size_t len);

/** Read len bytes while sending 0x00. */
void cim_spi_read(const cim_spi_dev_t *dev, uint8_t *rx, size_t len);

/** Full-duplex transfer of len bytes. */
void cim_spi_transfer(const cim_spi_dev_t *dev, const uint8_t *tx, uint8_t *rx, size_t len);

/* ---------------------------------- GPIO ---------------------------------- */

typedef enum {
    CIM_GPIO_PULL_NONE,
    CIM_GPIO_PULL_UP,
    CIM_GPIO_PULL_DOWN,
} cim_gpio_pull_t;

void cim_gpio_init_out(uint8_t pin, bool value);
void cim_gpio_init_in(uint8_t pin, cim_gpio_pull_t pull);
void cim_gpio_put(uint8_t pin, bool value);
bool cim_gpio_get(uint8_t pin);

/* ------------------------------ GPIO interrupts --------------------------- */

typedef enum {
    CIM_GPIO_EDGE_FALL = 1u << 0,
    CIM_GPIO_EDGE_RISE = 1u << 1,
} cim_gpio_edge_t;

/** Interrupt callback, runs in interrupt context: keep it short. */
typedef void (*cim_gpio_irq_cb_t)(uint8_t pin, void *ctx);

/**
 * Call cb on the given edge(s) of pin. Only one callback per pin.
 * Coexists with the Pico SDK's gpio_set_irq_callback() for other pins.
 * Returns false if pin is invalid.
 */
bool cim_gpio_irq_enable(uint8_t pin, unsigned edges, cim_gpio_irq_cb_t cb, void *ctx);

/** Disable the interrupt and remove the callback for pin. */
void cim_gpio_irq_disable(uint8_t pin);

/* --------------------------------- Delay ---------------------------------- */

/** Busy-wait for us microseconds. */
void cim_delay_us(uint32_t us);

/**
 * Wait for ms milliseconds.
 * Weak: an RTOS layer can override it to yield instead of blocking.
 */
void cim_delay_ms(uint32_t ms);

/* --------------------------------- Flash ---------------------------------- */

#define CIM_FLASH_SECTOR_SIZE 4096u /**< erase granularity */
#define CIM_FLASH_PAGE_SIZE   256u  /**< program granularity */

/** Size of the flash in bytes. */
uint32_t cim_flash_size(void);

/** Pointer to memory-mapped flash at offset (from the start of flash). */
const uint8_t *cim_flash_ptr(uint32_t offset);

/** Erase len bytes at offset. Both must be multiples of CIM_FLASH_SECTOR_SIZE. */
bool cim_flash_erase(uint32_t offset, uint32_t len);

/**
 * Program len bytes at offset. Both must be multiples of CIM_FLASH_PAGE_SIZE,
 * and the area must be erased.
 */
bool cim_flash_program(uint32_t offset, const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* CIM_HAL_H */
