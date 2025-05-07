#include "peripherals.hpp"

#include "TaskCPP.h"
#include "deca_device_api.h"
#include "deca_regs.h"

LED sysLed(GPIO::Port::C, GPIO::Pin::PIN_13);

UartConfig uart3Conf(uart3_info);
UartConfig uart6Conf(uart6_info);
Uart rs232(uart3Conf);

Uart uart6(uart6Conf);
Rs485 rs485(uart6, GPIO::Port::F, GPIO::Pin::PIN_4);

Logger Log(rs232);

std::vector<GPIO> HarnessGpio::condGpioArray;

static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_2048,   /* Preamble length. Used in TX only. */
    DWT_PAC64,       /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    0,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_110K,     /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (2049 + 64 - 64) /* SFD timeout (preamble length + 1 + SFD length - PAC
                        size). Used in RX only. */
};

void spi_sendrecv_bytes(uint8_t *tx_buf, uint8_t *rx_buf, uint16_t len) {
    gpio_bit_reset(GPIOB, GPIO_PIN_12);    // 手动拉低NSS

    for (uint16_t i = 0; i < len; i++) {
        while (spi_i2s_flag_get(SPI1, SPI_FLAG_TBE) ==
               RESET);    // 等待发送缓冲区空
        spi_i2s_data_transmit(SPI1, tx_buf[i]);
        while (spi_i2s_flag_get(SPI1, SPI_FLAG_RBNE) ==
               RESET);    // 等待接收完成
        rx_buf[i] = spi_i2s_data_receive(SPI1);
    }
    gpio_bit_set(GPIOB, GPIO_PIN_12);    // 手动拉高NSS
}

void port_set_dw1000_fastrate(void) {
    spi_disable(SPI1);

    spi_parameter_struct spi_init_struct;
    spi_struct_para_init(&spi_init_struct);
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.prescale = SPI_PSC_4;    // 60MHz /4 = 15MHz 好像不宜高于18
    spi_init(SPI1, &spi_init_struct);

    spi_enable(SPI1);
}

void reset_DW1000(void) {
    gpio_bit_reset(GPIOD, GPIO_PIN_5);    // reset pin
    TaskBase::delay(5);                   // hold
    gpio_bit_set(GPIOD, GPIO_PIN_5);
}

void spi1_init() {
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_SPI1);
    spi_parameter_struct spi_init_struct;

    // PB12 NSS  PB13 CLK   PB14 MISO   PB15 MOSI
    gpio_af_set(GPIOB, GPIO_AF_5, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO_PIN_12);    // NSS
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_12);

    gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO_PIN_5);    // RST
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_5);

    gpio_bit_set(GPIOB, GPIO_PIN_12);

    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = SPI_PSC_32;    // 这里spi主频为60MHz
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(SPI1, &spi_init_struct);

    spi_nss_output_disable(SPI1);
    spi_enable(SPI1);
}

void dw1000_init() {
    spi1_init();
    reset_DW1000();

    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR) {
        printf("dw1000 init failed");
        while (1) {
        };
    }
    port_set_dw1000_fastrate();    // spi freq: 1.875MHz ->15MHz
    dwt_configure(&config);

    dwt_write32bitreg(TX_POWER_ID, 0x85858585);
}

static uint8_t tx_msg[] = {
    0xC5,    // Frame control: blink frame
    0x00,    // Sequence number
    0xDE, 0xCA, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x00    // CRC
};
#define BLINK_FRAME_SN_IDX 1

void dw1000_send() {
    dwt_writetxdata(sizeof(tx_msg), tx_msg, 0);
    dwt_writetxfctrl(sizeof(tx_msg), 0, 0);

    dwt_starttx(DWT_START_TX_IMMEDIATE);
    while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS)) {
    };

    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
    tx_msg[BLINK_FRAME_SN_IDX]++;
}

#define FRAME_LEN_MAX 127
static uint8_t rx_buffer[FRAME_LEN_MAX];
static uint32_t status_reg = 0;
static uint16_t frame_len = 0;
uint32_t reg_diag = 0;
void dw1000_recv() {
    int i;
    /* Clear the RX buffer. */
    for (i = 0; i < FRAME_LEN_MAX; i++) {
        rx_buffer[i] = 0;
    }

    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_GOOD |
                                         SYS_STATUS_ALL_RX_ERR |
                                         SYS_STATUS_ALL_RX_TO);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    TaskBase::delay(1);

    while (
        !((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) &
          (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO))) {
    };

    if (status_reg & SYS_STATUS_RXFCG) {
        frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
        if (frame_len <= FRAME_LEN_MAX) {
            dwt_readrxdata(rx_buffer, frame_len, 0);
            frame_len -= 2;    // del CRC
        }
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
    } else if (status_reg & SYS_STATUS_ALL_RX_TO) {
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO);
        // printf("RX TIMEOUT!");
    } else if (status_reg & SYS_STATUS_ALL_RX_ERR) {
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        // printf("RX ERROR!");
    }
}