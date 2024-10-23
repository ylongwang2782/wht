#include "com.h"

#include <cstdint>

// 构造函数
UsartCom::UsartCom(uint8_t &idle_dma_rx_count) : rx_count(idle_dma_rx_count) {}

void UsartCom::init(uint32_t baudval) {
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_USART1);
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_5);
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_6);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_5);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_5);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_6);

    usart_deinit(USART1);
    usart_baudrate_set(USART1, baudval);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(USART1, USART_RECEIVE_DMA_ENABLE);
    usart_enable(USART1);

    usart_interrupt_enable(USART1, USART_INT_IDLE);
    UsartCom::dma_tx_config();
    UsartCom::idle_dma_rx_config();
}

void UsartCom::dma_tx_config() {
    uint8_t com1_txbuffer[1] = {0x01};
    dma_single_data_parameter_struct dma_init_struct;
    rcu_periph_clock_enable(RCU_DMA0);
    dma_deinit(DMA0, DMA_CH6);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uintptr_t)com1_txbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = ARRAYNUM(txbuffer);
    dma_init_struct.periph_addr = (uintptr_t)&USART_DATA(USART1);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH6, &dma_init_struct);

    dma_circulation_disable(DMA0, DMA_CH6);
    dma_channel_subperipheral_select(DMA0, DMA_CH6, DMA_SUBPERI4);
    dma_channel_disable(DMA0, DMA_CH6);
    usart_dma_transmit_config(USART1, USART_TRANSMIT_DMA_ENABLE);
}

void UsartCom::dma_tx(uint8_t *data, uint16_t len) {
    dma_channel_disable(DMA0, DMA_CH6);
    dma_flag_clear(DMA0, DMA_CH6, DMA_FLAG_FTF);
    dma_memory_address_config(DMA0, DMA_CH6, DMA_MEMORY_0, (uintptr_t)data);
    dma_transfer_number_config(DMA0, DMA_CH6, len);
    dma_channel_enable(DMA0, DMA_CH6);
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC));
}

void UsartCom::data_send(uint8_t *data, uint16_t len) {
    for (size_t i = 0; i < len; i++) {
        usart_data_transmit(USART1, data[i]);
        while (RESET == usart_flag_get(USART1, USART_FLAG_TBE)) {
        }
    }
}

void UsartCom::idle_dma_rx_config() {
    dma_single_data_parameter_struct dma_init_struct;
    nvic_irq_enable(USART1_IRQn, COM1_IRQ_PRE_PRIO, COM1_IRQ_SUB_PRIO);
    rcu_periph_clock_enable(RCU_DMA0);

    dma_deinit(DMA0, DMA_CH5);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uintptr_t)rxbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number = com_idle_rx_size;
    dma_init_struct.periph_addr = (uintptr_t)&USART_DATA(USART1);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_init_struct);

    dma_circulation_disable(DMA0, DMA_CH5);
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI4);
    dma_channel_enable(DMA0, DMA_CH5);

    usart_interrupt_enable(USART1, USART_INT_IDLE);
}

void UsartCom::idle_dma_rx_check(void) {
}