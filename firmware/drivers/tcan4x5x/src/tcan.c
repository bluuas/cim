/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Thin API for the TCAN4x5x, see tcan.h. The configuration sequence follows
 * TI's demo (Init_CAN() in TCAN455x Driver Library Demo 1.2.2).
 */

#include "tcan.h"

#include <string.h>

#include "TCAN4550.h"

/* ---------------------------------------------------------------------------
 * MRAM layout (2 KB): RX FIFO 0 with 64 byte elements (72 B each), generic TX
 * buffers (72 B each), standard filters (4 B each), extended filters (8 B each).
 * 16 * 72 + 8 * 72 + 16 * 4 + 16 * 8 = 1920 B
 * ------------------------------------------------------------------------- */
#define RX_FIFO_ELEMENTS 16
#define TX_BUFFERS       8

#define DEVICE_ID0_TCAN 0x4E414354u /* "TCAN", LSB first */

/* register bits not defined by name in TI's headers */
#define TXFQS_TFQF        (1u << 21) /* TX FIFO/queue full */
#define TXFQS_TFQPI_SHIFT 16         /* TX FIFO/queue put index */
#define TXFQS_TFQPI_MASK  0x1Fu
#define TXBC_TFQM         (1u << 30) /* 1 = queue mode, 0 = FIFO mode */
#define RXF0S_F0FL_MASK   0x7Fu      /* RX FIFO 0 fill level */
#define PSR_EP            (1u << 5)
#define PSR_EW            (1u << 6)
#define PSR_BO            (1u << 7)

/* TI's MRAM configuration cache (TCAN4550.c), needed to switch to TX FIFO mode */
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
extern uint32_t TCAN4x5x_MCAN_CACHE[9];
#endif

static struct {
    tcan_config_t config;
    volatile bool irq_pending;
    tcan_msg_t rx_buf[TCAN_RX_BUFFER_SIZE];
    unsigned rx_head; /* next slot to write */
    unsigned rx_tail; /* next slot to read */
    uint32_t rx_lost;
    uint32_t rx_overflows;
} tcan;

static const uint8_t dlc_len[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

uint8_t tcan_dlc_to_len(uint8_t dlc)
{
    return dlc_len[dlc & 0x0F];
}

uint8_t tcan_len_to_dlc(uint8_t len)
{
    for (uint8_t dlc = 0; dlc < 16; dlc++) {
        if (dlc_len[dlc] == len) {
            return dlc;
        }
    }
    return 0xFF;
}

/* ------------------------------- bit timing ------------------------------- */

/*
 * Find the smallest prescaler that divides the clock exactly into a bit with a
 * number of time quanta within [tq_min, tq_max], and split it at the sample point.
 */
static bool calc_timing(uint32_t clock_hz, uint32_t bitrate, unsigned prescaler_max, unsigned tq_min, unsigned tq_max,
                        unsigned sample_point_pct, unsigned before_max, unsigned after_max, unsigned after_min,
                        unsigned *prescaler, unsigned *before, unsigned *after)
{
    if (bitrate == 0) {
        return false;
    }
    for (unsigned p = 1; p <= prescaler_max; p++) {
        uint32_t div = p * bitrate;
        if (clock_hz % div != 0) {
            continue;
        }
        unsigned tq = clock_hz / div;
        if (tq > tq_max) {
            continue;
        }
        if (tq < tq_min) {
            return false; /* larger prescalers only make it worse */
        }
        unsigned b = (tq * sample_point_pct + 50) / 100;
        unsigned a = tq - b;
        if (a < after_min) {
            a = after_min;
            b = tq - a;
        }
        if (b > before_max || a > after_max || b < 2) {
            continue;
        }
        *prescaler = p;
        *before = b;
        *after = a;
        return true;
    }
    return false;
}

static bool configure_timing(const tcan_config_t *cfg)
{
    unsigned p, b, a;

    /* nominal: up to 80 tq, sample point 80 % */
    if (!calc_timing(cfg->clock_hz, cfg->nominal_bitrate, 512, 8, 80, 80, 257, 128, 2, &p, &b, &a)) {
        return false;
    }
    TCAN4x5x_MCAN_Nominal_Timing_Simple nom = {0};
    nom.NominalBitRatePrescaler = p;
    nom.NominalTqBeforeSamplePoint = b;
    nom.NominalTqAfterSamplePoint = a;
    if (!TCAN4x5x_MCAN_ConfigureNominalTiming_Simple(&nom)) {
        return false;
    }

    if (cfg->data_bitrate == 0) {
        return true;
    }
    /* data: up to 25 tq, sample point 75 % */
    if (!calc_timing(cfg->clock_hz, cfg->data_bitrate, 32, 4, 25, 75, 33, 16, 1, &p, &b, &a)) {
        return false;
    }
    TCAN4x5x_MCAN_Data_Timing_Simple data = {0};
    data.DataBitRatePrescaler = p;
    data.DataTqBeforeSamplePoint = b;
    data.DataTqAfterSamplePoint = a;
    return TCAN4x5x_MCAN_ConfigureDataTiming_Simple(&data);
}

/* --------------------------------- filters -------------------------------- */

static void count_filters(const tcan_config_t *cfg, uint8_t *num_std, uint8_t *num_ext)
{
    *num_std = 0;
    *num_ext = 0;
    for (uint8_t i = 0; i < cfg->num_filters; i++) {
        if (cfg->filters[i].ext) {
            (*num_ext)++;
        } else {
            (*num_std)++;
        }
    }
}

static bool write_filters(const tcan_config_t *cfg)
{
    uint8_t std_index = 0;
    uint8_t ext_index = 0;
    for (uint8_t i = 0; i < cfg->num_filters; i++) {
        const tcan_filter_t *f = &cfg->filters[i];
        if (f->ext) {
            TCAN4x5x_MCAN_XID_Filter xid = {0};
            xid.EFT = TCAN4x5x_XID_EFT_CLASSIC;
            xid.EFEC = TCAN4x5x_XID_EFEC_STORERX0;
            xid.EFID1 = f->id & 0x1FFFFFFF;
            xid.EFID2 = f->mask & 0x1FFFFFFF;
            if (!TCAN4x5x_MCAN_WriteXIDFilter(ext_index++, &xid)) {
                return false;
            }
        } else {
            TCAN4x5x_MCAN_SID_Filter sid = {0};
            sid.SFT = TCAN4x5x_SID_SFT_CLASSIC;
            sid.SFEC = TCAN4x5x_SID_SFEC_STORERX0;
            sid.SFID1 = f->id & 0x7FF;
            sid.SFID2 = f->mask & 0x7FF;
            if (!TCAN4x5x_MCAN_WriteSIDFilter(std_index++, &sid)) {
                return false;
            }
        }
    }
    return true;
}

/* ------------------------------ MCAN config ------------------------------- */

static bool set_tx_fifo_mode(void)
{
    uint32_t txbc = AHB_READ_32(REG_MCAN_TXBC) & ~TXBC_TFQM;
    AHB_WRITE_32(REG_MCAN_TXBC, txbc);
#ifdef TCAN4x5x_MCAN_CACHE_CONFIGURATION
    TCAN4x5x_MCAN_CACHE[TCAN4x5x_MCAN_CACHE_TXBC] = txbc;
#endif
    return AHB_READ_32(REG_MCAN_TXBC) == txbc;
}

static bool configure_mcan(const tcan_config_t *cfg)
{
    uint8_t num_std, num_ext;
    count_filters(cfg, &num_std, &num_ext);

    TCAN4x5x_MCAN_CCCR_Config cccr = {0};
    cccr.FDOE = cfg->data_bitrate != 0;
    cccr.BRSE = cfg->data_bitrate != 0;

    TCAN4x5x_MCAN_Global_Filter_Configuration gfc = {0};
    gfc.RRFE = 1; /* reject remote frames */
    gfc.RRFS = 1;
    gfc.ANFE = cfg->num_filters ? TCAN4x5x_GFC_REJECT : TCAN4x5x_GFC_ACCEPT_INTO_RXFIFO0;
    gfc.ANFS = cfg->num_filters ? TCAN4x5x_GFC_REJECT : TCAN4x5x_GFC_ACCEPT_INTO_RXFIFO0;

    TCAN4x5x_MRAM_Config mram = {0};
    mram.SIDNumElements = num_std;
    mram.XIDNumElements = num_ext;
    mram.Rx0NumElements = RX_FIFO_ELEMENTS;
    mram.Rx0ElementSize = MRAM_64_Byte_Data;
    mram.Rx1NumElements = 0;
    mram.Rx1ElementSize = MRAM_64_Byte_Data;
    mram.RxBufNumElements = 0;
    mram.RxBufElementSize = MRAM_64_Byte_Data;
    mram.TxEventFIFONumElements = 0;
    mram.TxBufferNumElements = TX_BUFFERS;
    mram.TxBufferElementSize = MRAM_64_Byte_Data;

    /* protected registers: unlock once, configure everything, lock again */
    bool ok = TCAN4x5x_MCAN_EnableProtectedRegisters();
    ok = ok && TCAN4x5x_MCAN_ConfigureCCCRRegister(&cccr);
    ok = ok && TCAN4x5x_MCAN_ConfigureGlobalFilter(&gfc);
    ok = ok && configure_timing(cfg);
    if (ok) {
        TCAN4x5x_MRAM_Clear();
    }
    ok = ok && TCAN4x5x_MRAM_Configure(&mram);
    if (ok && cfg->tx_mode == TCAN_TX_FIFO) {
        ok = set_tx_fifo_mode();
    }
    ok = TCAN4x5x_MCAN_DisableProtectedRegisters() && ok;
    if (!ok) {
        return false;
    }

    TCAN4x5x_MCAN_Interrupt_Enable ie = {0};
    ie.RF0NE = 1; /* RX FIFO 0 new message */
    ie.RF0LE = 1; /* RX FIFO 0 message lost */
    ie.BOE = 1;   /* bus-off status changed */
    TCAN4x5x_MCAN_ConfigureInterruptEnable(&ie);

    return write_filters(cfg);
}

static bool configure_device(const tcan_config_t *cfg)
{
    TCAN4x5x_DEV_CONFIG dev = {0};
    dev.SWE_DIS = 0;
    dev.DEVICE_RESET = 0;
    dev.WD_EN = 0;
    dev.nWKRQ_CONFIG = 0;
    dev.INH_DIS = 0;
    dev.GPIO1_GPO_CONFIG = TCAN4x5x_DEV_CONFIG_GPO1_MCAN_INT1;
    dev.FAIL_SAFE_EN = 0;
    dev.GPIO1_CONFIG = TCAN4x5x_DEV_CONFIG_GPIO1_CONFIG_GPO;
    dev.WD_ACTION = TCAN4x5x_DEV_CONFIG_WDT_ACTION_nINT;
    dev.WD_BIT_RESET = 0;
    dev.nWKRQ_VOLTAGE = 0;
    dev.GPO2_CONFIG = TCAN4x5x_DEV_CONFIG_GPO2_NO_ACTION;
    dev.CLK_REF = cfg->clock_hz == 40000000 ? 1 : 0; /* 1: 40 MHz, 0: 20 MHz */
    dev.WAKE_CONFIG = TCAN4x5x_DEV_CONFIG_WAKE_BOTH_EDGES;
    return TCAN4x5x_Device_Configure(&dev);
}

/* ---------------------------------- init ---------------------------------- */

static void on_nint(uint8_t pin, void *ctx)
{
    (void)pin;
    (void)ctx;
    tcan.irq_pending = true;
}

static bool valid_config(const tcan_config_t *cfg)
{
    if (cfg->clock_hz != 40000000 && cfg->clock_hz != 20000000) {
        return false;
    }
    if (cfg->num_filters > 0 && cfg->filters == NULL) {
        return false;
    }
    uint8_t num_std, num_ext;
    count_filters(cfg, &num_std, &num_ext);
    return num_std <= TCAN_MAX_FILTERS && num_ext <= TCAN_MAX_FILTERS;
}

tcan_err_t tcan_init(const tcan_config_t *config)
{
    if (config == NULL || !valid_config(config)) {
        return TCAN_ERR_PARAM;
    }
    memset(&tcan, 0, sizeof tcan);
    tcan.config = *config;
    const tcan_config_t *cfg = &tcan.config;

    TCAN4x5x_SPI_Init(&cfg->spi);
    cim_gpio_init_in(cfg->nint_pin, CIM_GPIO_PULL_UP);

    /* hardware reset: pulse RST high, then wait for the device to start */
    cim_gpio_init_out(cfg->rst_pin, false);
    cim_gpio_put(cfg->rst_pin, true);
    cim_delay_us(50);
    cim_gpio_put(cfg->rst_pin, false);
    cim_delay_ms(5);

    if (AHB_READ_32(REG_SPI_DEVICE_ID0) != DEVICE_ID0_TCAN) {
        return TCAN_ERR_DEVICE;
    }

    /* device interrupts off, clear what is pending (power-on etc.) */
    TCAN4x5x_Device_ClearSPIERR();
    TCAN4x5x_Device_Interrupt_Enable dev_ie = {0};
    if (!TCAN4x5x_Device_ConfigureInterruptEnable(&dev_ie)) {
        return TCAN_ERR_CONFIG;
    }
    TCAN4x5x_Device_ClearInterruptsAll();

    if (!configure_mcan(cfg) || !configure_device(cfg)) {
        return TCAN_ERR_CONFIG;
    }

    TCAN4x5x_Device_Interrupts dev_ir;
    TCAN4x5x_Device_ReadInterrupts(&dev_ir);
    if (dev_ir.UVSUP) {
        return TCAN_ERR_NO_VSUP;
    }
    if (!TCAN4x5x_Device_SetMode(TCAN4x5x_DEVICE_MODE_NORMAL)) {
        return TCAN_ERR_MODE;
    }

    TCAN4x5x_MCAN_ClearInterruptsAll();
    cim_gpio_irq_enable(cfg->nint_pin, CIM_GPIO_EDGE_FALL, on_nint, NULL);
    tcan.irq_pending = true; /* handle anything that arrived during init */
    return TCAN_OK;
}

/* ----------------------------------- TX ----------------------------------- */

tcan_err_t tcan_send(const tcan_msg_t *msg)
{
    if (msg == NULL) {
        return TCAN_ERR_PARAM;
    }
    uint8_t dlc = tcan_len_to_dlc(msg->len);
    bool fd = msg->fd && tcan.config.data_bitrate != 0;
    if (dlc == 0xFF || (!fd && msg->len > 8) || (msg->fd && !fd) || msg->id > (msg->ext ? 0x1FFFFFFFu : 0x7FFu)) {
        return TCAN_ERR_PARAM;
    }

    if (AHB_READ_32(REG_MCAN_PSR) & PSR_BO) {
        return TCAN_ERR_BUS_OFF;
    }
    uint32_t txfqs = AHB_READ_32(REG_MCAN_TXFQS);
    if (txfqs & TXFQS_TFQF) {
        return TCAN_ERR_BUSY;
    }
    uint8_t index = (txfqs >> TXFQS_TFQPI_SHIFT) & TXFQS_TFQPI_MASK;

    TCAN4x5x_MCAN_TX_Header header = {0};
    header.ID = msg->id;
    header.XTD = msg->ext;
    header.FDF = fd;
    header.BRS = fd && msg->brs;
    header.DLC = dlc;

    /* TI's function does not modify the payload, it just lacks const */
    if (TCAN4x5x_MCAN_WriteTXBuffer(index, &header, (uint8_t *)msg->data) == 0) {
        return TCAN_ERR_PARAM;
    }
    TCAN4x5x_MCAN_TransmitBufferContents(index);
    return TCAN_OK;
}

/* ----------------------------------- RX ----------------------------------- */

static void rx_push(const TCAN4x5x_MCAN_RX_Header *header, const uint8_t *data)
{
    unsigned next = (tcan.rx_head + 1) % TCAN_RX_BUFFER_SIZE;
    if (next == tcan.rx_tail) {
        tcan.rx_overflows++;
        return;
    }
    tcan_msg_t *msg = &tcan.rx_buf[tcan.rx_head];
    msg->id = header->ID;
    msg->ext = header->XTD;
    msg->fd = header->FDF;
    msg->brs = header->BRS;
    /* length from the DLC, not from TI's return value (uninitialised for DLC 0, see #34) */
    msg->len = tcan_dlc_to_len(header->DLC);
    msg->timestamp = header->RXTS;
    memcpy(msg->data, data, msg->len);
    tcan.rx_head = next;
}

unsigned tcan_poll(void)
{
    /* nINT is level-active: also check the pin in case an edge was missed */
    if (!tcan.irq_pending && cim_gpio_get(tcan.config.nint_pin)) {
        return 0;
    }
    tcan.irq_pending = false;

    TCAN4x5x_MCAN_Interrupts ir;
    TCAN4x5x_MCAN_ReadInterrupts(&ir);
    TCAN4x5x_MCAN_ClearInterrupts(&ir);
    if (ir.RF0L) {
        tcan.rx_lost++;
    }

    TCAN4x5x_Device_Interrupts dev_ir;
    TCAN4x5x_Device_ReadInterrupts(&dev_ir);
    if (dev_ir.SPIERR) {
        TCAN4x5x_Device_ClearSPIERR();
    }

    unsigned received = 0;
    unsigned fill = AHB_READ_32(REG_MCAN_RXF0S) & RXF0S_F0FL_MASK;
    while (fill--) {
        TCAN4x5x_MCAN_RX_Header header = {0};
        uint8_t data[64];
        (void)TCAN4x5x_MCAN_ReadNextFIFO(RXFIFO0, &header, data);
        rx_push(&header, data);
        received++;
    }
    return received;
}

bool tcan_receive(tcan_msg_t *msg)
{
    if (tcan.rx_tail == tcan.rx_head) {
        return false;
    }
    *msg = tcan.rx_buf[tcan.rx_tail];
    tcan.rx_tail = (tcan.rx_tail + 1) % TCAN_RX_BUFFER_SIZE;
    return true;
}

/* --------------------------------- status --------------------------------- */

void tcan_get_status(tcan_status_t *status)
{
    uint32_t psr = AHB_READ_32(REG_MCAN_PSR);
    uint32_t ecr = AHB_READ_32(REG_MCAN_ECR);
    TCAN4x5x_Device_Interrupts dev_ir;
    TCAN4x5x_Device_ReadInterrupts(&dev_ir);

    status->bus_off = psr & PSR_BO;
    status->error_passive = psr & PSR_EP;
    status->error_warning = psr & PSR_EW;
    status->tx_errors = ecr & 0xFF;
    status->rx_errors = (ecr >> 8) & 0x7F;
    status->vsup_ok = !dev_ir.UVSUP;
    status->rx_lost = tcan.rx_lost;
    status->rx_overflows = tcan.rx_overflows;
}
