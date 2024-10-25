#include "wht_usart.h"

#include "gd32f4xx_dma.h"

#define ARRAYNUM(arr_name)  (uint32_t)(sizeof(arr_name) / sizeof(*(arr_name)))
#define USART0_DATA_ADDRESS ((uint32_t) & USART_DATA(USART0))
#define USART1_DATA_ADDRESS ((uint32_t) & USART_DATA(USART1))
#define USART2_DATA_ADDRESS ((uint32_t) & USART_DATA(USART2))
#define com_idle_rx_size    256

#define COM0_IRQ_PRE_PRIO   1
#define COM0_IRQ_SUB_PRIO   0
#define COM1_IRQ_PRE_PRIO   1
#define COM1_IRQ_SUB_PRIO   1
#define COM2_IRQ_PRE_PRIO   1
#define COM2_IRQ_SUB_PRIO   2

static rcu_periph_enum COM_CLK[COMn] = {WHT_COM0_CLK, WHT_COM1_CLK,
                                        WHT_COM2_CLK};
static uint32_t COM_GPIO_CLK[COMn] = {WHT_COM0_GPIO_CLK, WHT_COM1_GPIO_CLK,
                                      WHT_COM2_GPIO_CLK};
static uint32_t COM_TX_PORT[COMn] = {WHT_COM0_GPIO_PORT, WHT_COM1_GPIO_PORT,
                                     WHT_COM2_GPIO_PORT};
static uint32_t COM_AF[COMn] = {WHT_COM0_AF, WHT_COM1_AF, WHT_COM2_AF};
static uint32_t COM_TX_PIN[COMn] = {WHT_COM0_TX_PIN, WHT_COM1_TX_PIN,
                                    WHT_COM2_TX_PIN};
static uint32_t COM_RX_PIN[COMn] = {WHT_COM0_RX_PIN, WHT_COM1_RX_PIN,
                                    WHT_COM2_RX_PIN};

void (*usart0_callback)(void) = NULL;
uint8_t com0_txbuffer[1] = {0x01};
uint8_t com0_rxbuffer[com_idle_rx_size];

void (*usart2_callback)(void) = NULL;
uint8_t com2_txbuffer[1] = {0x01};
uint8_t com2_rxbuffer[com_idle_rx_size];

void wht_com_init(uint32_t com) {
    uint32_t COM_ID = 0;
    dma_single_data_parameter_struct dma_init_struct;
    if (WHT_COM0 == com) {
        COM_ID = 0U;
    } else if (WHT_COM1 == com) {
        COM_ID = 1U;
    } else if (WHT_COM2 == com) {
        COM_ID = 2U;
    }

    rcu_periph_clock_enable(COM_GPIO_CLK[COM_ID]);
    /* enable USART clock */
    rcu_periph_clock_enable(COM_CLK[COM_ID]);
    /* connect port to USARTx_Tx */
    gpio_af_set(COM_TX_PORT[COM_ID], COM_AF[COM_ID], COM_TX_PIN[COM_ID]);
    /* connect port to USARTx_Rx */
    gpio_af_set(COM_TX_PORT[COM_ID], COM_AF[COM_ID], COM_RX_PIN[COM_ID]);
    /* configure USART Tx as alternate function push-pull */
    gpio_mode_set(COM_TX_PORT[COM_ID], GPIO_MODE_AF, GPIO_PUPD_PULLUP,
                  COM_TX_PIN[COM_ID]);
    gpio_output_options_set(COM_TX_PORT[COM_ID], GPIO_OTYPE_PP,
                            GPIO_OSPEED_50MHZ, COM_TX_PIN[COM_ID]);
    /* configure USART Rx as alternate function push-pull */
    gpio_mode_set(COM_TX_PORT[COM_ID], GPIO_MODE_AF, GPIO_PUPD_PULLUP,
                  COM_RX_PIN[COM_ID]);
    gpio_output_options_set(COM_TX_PORT[COM_ID], GPIO_OTYPE_PP,
                            GPIO_OSPEED_50MHZ, COM_RX_PIN[COM_ID]);

    /* USART configure */
    usart_deinit(com);
    usart_baudrate_set(com, 115200U);
    usart_receive_config(com, USART_RECEIVE_ENABLE);
    usart_transmit_config(com, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(com, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(com, USART_RECEIVE_DMA_ENABLE);
    usart_enable(com);

    usart_interrupt_enable(com, USART_INT_IDLE);
}

void wht_com_send(uint32_t usart_periph, uint8_t *data, uint16_t len) {
    int i;
    for (i = 0; i < len; i++) {
        usart_data_transmit(usart_periph, data[i]);
        while (RESET == usart_flag_get(usart_periph, USART_FLAG_TBE)) {
        }
    }
}

void wht_com0_dma_tx_config(void) {
    dma_single_data_parameter_struct dma_init_struct;
    /* enable DMA1 */
    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(DMA1, DMA_CH2);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)com0_txbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = ARRAYNUM(com0_txbuffer);
    dma_init_struct.periph_addr = USART0_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH2, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(DMA1, DMA_CH2);
    dma_channel_subperipheral_select(DMA1, DMA_CH2, DMA_SUBPERI4);
    /* enable DMA channel7 */
    dma_channel_disable(DMA1, DMA_CH2);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
}

void wht_com0_dma_tx(uint8_t *data, uint16_t len) {
    dma_channel_disable(DMA1, DMA_CH2);
    dma_flag_clear(DMA1, DMA_CH2, DMA_FLAG_FTF);
    dma_memory_address_config(DMA1, DMA_CH2, DMA_MEMORY_0, (uint32_t)data);
    dma_transfer_number_config(DMA1, DMA_CH2, len);
    dma_channel_enable(DMA1, DMA_CH2);
    while (RESET == usart_flag_get(WHT_COM1, USART_FLAG_TC));
}

void wht_com0_idle_dma_rx_config(uint32_t com, void (*callback)(void)) {
    dma_single_data_parameter_struct dma_init_struct;

    nvic_irq_enable(USART0_IRQn, COM0_IRQ_PRE_PRIO, COM0_IRQ_SUB_PRIO);

    rcu_periph_clock_enable(RCU_DMA1);

    dma_deinit(DMA1, DMA_CH2);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)com0_rxbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number = com_idle_rx_size;
    dma_init_struct.periph_addr = USART0_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH2, &dma_init_struct);

    /* configure DMA mode */
    dma_circulation_disable(DMA1, DMA_CH2);
    dma_channel_subperipheral_select(DMA1, DMA_CH2, DMA_SUBPERI4);
    /* enable DMA0 channel2 */
    dma_channel_enable(DMA1, DMA_CH2);

    usart_interrupt_enable(com, USART_INT_IDLE);
    usart0_callback = callback;
}

extern void (*usart1_callback)(void);
uint8_t com1_txbuffer[1] = {0x01};
uint8_t com1_rxbuffer[com_idle_rx_size];
void com1_init(uint32_t baudval) {
    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART1);
    /* connect port to USARTx_Tx */
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_5);
    /* connect port to USARTx_Rx */
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_6);
    /* configure USART Tx as alternate function push-pull */
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_5);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_5);
    /* configure USART Rx as alternate function push-pull */
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_6);

    /* USART configure */
    usart_deinit(USART1);
    usart_baudrate_set(USART1, baudval);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(USART1, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(USART1, USART_RECEIVE_DMA_ENABLE);
    usart_enable(USART1);

    usart_interrupt_enable(USART1, USART_INT_IDLE);
}

void com1_dma_tx_config(void) {
    dma_single_data_parameter_struct dma_init_struct;
    /* enable DMA1 */
    rcu_periph_clock_enable(RCU_DMA0);
    dma_deinit(DMA0, DMA_CH6);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)com1_txbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = ARRAYNUM(com1_txbuffer);
    dma_init_struct.periph_addr = USART1_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH6, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH6);
    dma_channel_subperipheral_select(DMA0, DMA_CH6, DMA_SUBPERI4);
    /* enable DMA channel7 */
    dma_channel_disable(DMA0, DMA_CH6);
    usart_dma_transmit_config(USART1, USART_TRANSMIT_DMA_ENABLE);
}

void com1_dma_tx(uint8_t *data, uint16_t len) {
    dma_channel_disable(DMA0, DMA_CH6);
    dma_flag_clear(DMA0, DMA_CH6, DMA_FLAG_FTF);
    dma_memory_address_config(DMA0, DMA_CH6, DMA_MEMORY_0, (uint32_t)data);
    dma_transfer_number_config(DMA0, DMA_CH6, len);
    dma_channel_enable(DMA0, DMA_CH6);
    while (RESET == usart_flag_get(WHT_COM1, USART_FLAG_TC));
}

void com1_idle_dma_rx_config(uint32_t com, void (*callback)(void)) {
    dma_single_data_parameter_struct dma_init_struct;

    nvic_irq_enable(USART1_IRQn, COM1_IRQ_PRE_PRIO, COM1_IRQ_SUB_PRIO);

    rcu_periph_clock_enable(RCU_DMA0);

    dma_deinit(DMA0, DMA_CH5);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)com1_rxbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number = com_idle_rx_size;
    dma_init_struct.periph_addr = USART1_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_init_struct);

    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH5);
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI4);
    /* enable DMA0 channel2 */
    dma_channel_enable(DMA0, DMA_CH5);

    usart_interrupt_enable(com, USART_INT_IDLE);
    usart1_callback = callback;
}

void wht_com2_dma_tx_config(void) {
    dma_single_data_parameter_struct dma_init_struct;
    /* enable DMA1 */
    rcu_periph_clock_enable(RCU_DMA0);
    dma_deinit(DMA0, DMA_CH3);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)com2_txbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.number = ARRAYNUM(com2_txbuffer);
    dma_init_struct.periph_addr = USART2_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH3, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH3);
    dma_channel_subperipheral_select(DMA0, DMA_CH3, DMA_SUBPERI4);
    /* enable DMA channel7 */
    dma_channel_disable(DMA0, DMA_CH3);
    usart_dma_transmit_config(USART2, USART_TRANSMIT_DMA_ENABLE);
}

void wht_com2_dma_tx(uint8_t *data, uint16_t len) {
    dma_channel_disable(DMA0, DMA_CH3);
    dma_flag_clear(DMA0, DMA_CH3, DMA_FLAG_FTF);
    dma_memory_address_config(DMA0, DMA_CH3, DMA_MEMORY_0, (uint32_t)data);
    dma_transfer_number_config(DMA0, DMA_CH3, len);
    dma_channel_enable(DMA0, DMA_CH3);
    while (RESET == usart_flag_get(WHT_COM2, USART_FLAG_TC));
}

void wht_com2_idle_dma_rx_config(uint32_t com, void (*callback)(void)) {
    dma_single_data_parameter_struct dma_init_struct;

    nvic_irq_enable(USART2_IRQn, COM2_IRQ_PRE_PRIO, COM2_IRQ_SUB_PRIO);

    rcu_periph_clock_enable(RCU_DMA0);

    dma_deinit(DMA0, DMA_CH1);
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;
    dma_init_struct.memory0_addr = (uint32_t)com2_rxbuffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number = com_idle_rx_size;
    dma_init_struct.periph_addr = USART2_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH1, &dma_init_struct);

    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH1);
    dma_channel_subperipheral_select(DMA0, DMA_CH1, DMA_SUBPERI4);
    /* enable DMA0 channel2 */
    dma_channel_enable(DMA0, DMA_CH1);

    usart_interrupt_enable(com, USART_INT_IDLE);
    usart2_callback = callback;
}

int fputc(int ch, FILE *f) {
    usart_data_transmit(USART1, (uint8_t)ch);
    while (RESET == usart_flag_get(USART1, USART_FLAG_TBE)) {
    }
    return ch;
}