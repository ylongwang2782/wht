#include "bsp_spi.hpp"

const SPI_CONFIG SPI1_PC1MOSI_PC2MISO_PB10SCLK = {
    .spi_periph = SPI1,
    .spi_periph_clock = RCU_SPI1,

    .mosi_port = GPIOC,
    .mosi_pin = GPIO_PIN_1,
    .mosi_func_num = GPIO_AF_7,

    .miso_port = GPIOC,
    .miso_pin = GPIO_PIN_2,
    .miso_func_num = GPIO_AF_5,

    .sclk_port = GPIOB,
    .sclk_pin = GPIO_PIN_10,
    .sclk_func_num = GPIO_AF_5,
};
