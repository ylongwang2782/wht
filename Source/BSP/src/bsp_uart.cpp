#include "bsp_uart.hpp"

#include <cstdio>

UasrtInfo usart0_info = {.baudrate = 921600,
                         .gpio_port = GPIOB,
                         .tx_pin = GPIO_PIN_6,
                         .rx_pin = GPIO_PIN_7,
                         .usart_periph = USART0,
                         .usart_clk = RCU_USART0,
                         .usart_port_clk = RCU_GPIOB,
                         .gpio_af = GPIO_AF_7,
                         .rcu_dma_periph = RCU_DMA1,
                         .dma_periph = DMA1,
                         .dma_sub_per = DMA_SUBPERI4,
                         .dma_tx_channel = DMA_CH7,
                         .dma_rx_channel = DMA_CH2,
                         .nvic_irq = USART0_IRQn,
                         .nvic_irq_pre_priority = 1,
                         .nvic_irq_sub_priority = 3,
                         .rx_count = 0,
                         .dmaRxDoneSema = xSemaphoreCreateBinary()};

UasrtInfo usart1_info = {.baudrate = 115200,
                         .gpio_port = GPIOD,
                         .tx_pin = GPIO_PIN_5,
                         .rx_pin = GPIO_PIN_6,
                         .usart_periph = USART1,
                         .usart_clk = RCU_USART1,
                         .usart_port_clk = RCU_GPIOD,
                         .gpio_af = GPIO_AF_7,
                         .rcu_dma_periph = RCU_DMA0,
                         .dma_periph = DMA0,
                         .dma_sub_per = DMA_SUBPERI4,
                         .dma_tx_channel = DMA_CH6,
                         .dma_rx_channel = DMA_CH5,
                         .nvic_irq = USART1_IRQn,
                         .nvic_irq_pre_priority = 6,
                         .nvic_irq_sub_priority = 0,
                         .rx_count = 0,
                         .dmaRxDoneSema = xSemaphoreCreateBinary()};

UasrtInfo usart2_info = {.baudrate = 115200,
                         .gpio_port = GPIOB,
                         .tx_pin = GPIO_PIN_10,
                         .rx_pin = GPIO_PIN_11,
                         .usart_periph = USART2,
                         .usart_clk = RCU_USART2,
                         .usart_port_clk = RCU_GPIOB,
                         .gpio_af = GPIO_AF_7,
                         .rcu_dma_periph = RCU_DMA0,
                         .dma_periph = DMA0,
                         .dma_sub_per = DMA_SUBPERI4,
                         .dma_tx_channel = DMA_CH3,
                         .dma_rx_channel = DMA_CH1,
                         .nvic_irq = USART2_IRQn,
                         .nvic_irq_pre_priority = 1,
                         .nvic_irq_sub_priority = 2,
                         .rx_count = 0,
                         .dmaRxDoneSema = xSemaphoreCreateBinary()};

UasrtInfo uart3_info = {.baudrate = 921600,
                        .gpio_port = GPIOA,
                        .tx_pin = GPIO_PIN_0,
                        .rx_pin = GPIO_PIN_1,
                        .usart_periph = UART3,
                        .usart_clk = RCU_UART3,
                        .usart_port_clk = RCU_GPIOA,
                        .gpio_af = GPIO_AF_8,
                        .rcu_dma_periph = RCU_DMA0,
                        .dma_periph = DMA0,
                        .dma_sub_per = DMA_SUBPERI4,
                        .dma_tx_channel = DMA_CH4,
                        .dma_rx_channel = DMA_CH2,
                        .nvic_irq = UART3_IRQn,
                        .nvic_irq_pre_priority = 1,
                        .nvic_irq_sub_priority = 3,
                        .rx_count = 0,
                        .dmaRxDoneSema = xSemaphoreCreateBinary()};

UasrtInfo usart5_info = {.baudrate = 115200,
                         .gpio_port = GPIOC,
                         .tx_pin = GPIO_PIN_6,
                         .rx_pin = GPIO_PIN_7,
                         .usart_periph = USART5,
                         .usart_clk = RCU_USART5,
                         .usart_port_clk = RCU_GPIOC,
                         .gpio_af = GPIO_AF_8,
                         .rcu_dma_periph = RCU_DMA1,
                         .dma_periph = DMA1,
                         .dma_sub_per = DMA_SUBPERI5,
                         .dma_tx_channel = DMA_CH6,
                         .dma_rx_channel = DMA_CH2,
                         .nvic_irq = USART5_IRQn,
                         .nvic_irq_pre_priority = 7,
                         .nvic_irq_sub_priority = 0,
                         .rx_count = 0,
                         .dmaRxDoneSema = xSemaphoreCreateBinary()};

UasrtInfo uart6_info = {.baudrate = 115200,
                        .gpio_port = GPIOF,
                        .tx_pin = GPIO_PIN_6,
                        .rx_pin = GPIO_PIN_7,
                        .usart_periph = UART6,
                        .usart_clk = RCU_UART6,
                        .usart_port_clk = RCU_GPIOF,
                        .gpio_af = GPIO_AF_8,
                        .rcu_dma_periph = RCU_DMA0,
                        .dma_periph = DMA0,
                        .dma_sub_per = DMA_SUBPERI5,
                        .dma_tx_channel = DMA_CH1,
                        .dma_rx_channel = DMA_CH3,
                        .nvic_irq = UART6_IRQn,
                        .nvic_irq_pre_priority = 1,
                        .nvic_irq_sub_priority = 3,
                        .rx_count = 0,
                        .dmaRxDoneSema = xSemaphoreCreateBinary()};

UasrtInfo uart7_info = {.baudrate = 115200,
                        .gpio_port = GPIOE,
                        .tx_pin = GPIO_PIN_1,
                        .rx_pin = GPIO_PIN_0,
                        .usart_periph = UART7,
                        .usart_clk = RCU_UART7,
                        .usart_port_clk = RCU_GPIOE,
                        .gpio_af = GPIO_AF_8,
                        .rcu_dma_periph = RCU_DMA0,
                        .dma_periph = DMA0,
                        .dma_sub_per = DMA_SUBPERI5,
                        .dma_tx_channel = DMA_CH0,
                        .dma_rx_channel = DMA_CH6,
                        .nvic_irq = UART7_IRQn,
                        .nvic_irq_pre_priority = 3,
                        .nvic_irq_sub_priority = 3,
                        .rx_count = 0,
                        .dmaRxDoneSema = xSemaphoreCreateBinary()};

// 全局信号量
void handle_usart_interrupt(UasrtInfo *config) {
    if (config->use_dma) {
        if (RESET != usart_interrupt_flag_get(config->usart_periph,
                                              USART_INT_FLAG_IDLE)) {
            /* clear IDLE flag */
            usart_data_receive(config->usart_periph);
            /* number of data received */
            config->rx_count = DMA_RX_BUFFER_SIZE -
                               (dma_transfer_number_get(
                                   config->dma_periph, config->dma_rx_channel));
            dma_channel_disable(config->dma_periph, config->dma_rx_channel);
            dma_flag_clear(config->dma_periph, config->dma_rx_channel,
                           DMA_FLAG_FTF);
            // 通知任务 DMA 接收完成
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(config->dmaRxDoneSema,
                                  &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            dma_transfer_number_config(
                config->dma_periph, config->dma_rx_channel, DMA_RX_BUFFER_SIZE);
            dma_channel_enable(config->dma_periph, config->dma_rx_channel);
        }
    } else {
        if (RESET != usart_interrupt_flag_get(config->usart_periph,
                                              USART_INT_FLAG_RBNE)) {
            /* read one byte from the receive data register */
            usart_data_receive(config->usart_periph);
            // config->rx_buffer[config->rx_count++] =
            // usart_data_receive(config->usart_periph);
        }
    }
}

extern "C" {
void USART0_IRQHandler(void) { handle_usart_interrupt(&usart0_info); }
void USART1_IRQHandler(void) { handle_usart_interrupt(&usart1_info); }
void USART2_IRQHandler(void) { handle_usart_interrupt(&usart2_info); }
void UART3_IRQHandler(void) { handle_usart_interrupt(&uart3_info); }
void USART5_IRQHandler(void) { handle_usart_interrupt(&usart5_info); }
void UART6_IRQHandler(void) { handle_usart_interrupt(&uart6_info); }
void UART7_IRQHandler(void) { handle_usart_interrupt(&uart7_info); }
}
