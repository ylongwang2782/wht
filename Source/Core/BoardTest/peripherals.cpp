#include "peripherals.hpp"

LED sysLed(GPIO::Port::C, GPIO::Pin::PIN_13);

// UartConfig usart0Conf(usart0_info_PA9PA10);
UartConfig uart3Conf(uart3_info);
UartConfig uart6Conf(uart6_info);

// Uart usart0(usart0Conf);
Uart rs232(uart3Conf);
Uart uart6(uart6Conf);
Rs485 rs485(uart6, GPIO::Port::F, GPIO::Pin::PIN_4);

Logger Log(rs232);

std::vector<GPIO> HarnessGpio::condGpioArray;

dwt_config_t dw1000_config = {
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

Spi_IOConfig spi_cfg = {
    .spi_periph = SPI3,
    .spi_periph_clock = RCU_SPI3,

    .mosi_port = GPIOE,
    .mosi_pin = GPIO_PIN_6,
    .mosi_func_num = GPIO_AF_5,

    .miso_port = GPIOE,
    .miso_pin = GPIO_PIN_5,
    .miso_func_num = GPIO_AF_5,

    .sclk_port = GPIOE,
    .sclk_pin = GPIO_PIN_2,
    .sclk_func_num = GPIO_AF_5,

    .nss_port = GPIOE,
    .nss_pin = GPIO_PIN_4,
    .nss_func_num = GPIO_AF_5,
};