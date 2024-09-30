#include "slave_mode.h"

#include "wht_timer.h"
#include "wht_usart.h"

uint16_t wire_data = 0;
static int32_t heartbeat = -1;

void running_led_init(void);
void slave_timer_config(void);
void timer_task(void);
int wire_gpio_init(void);
void com_rx_idle_callback(void);

uint8_t tx_buffer[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
extern uint8_t com1_rxbuffer[256];
__IO uint8_t rx_count = 0;
__IO uint8_t tx_count = 0;
__IO uint8_t receive_flag = 0;

int slave_mode_entry(void) {
    rcu_periph_clock_enable(RCU_SYSCFG);

    systick_config();
    running_led_init();

    timer1_init(timer_task);

    wire_gpio_init();

    wht_com1_idle_rx_dma_config(WHT_COM1,com_rx_idle_callback);
    wht_com_init(WHT_COM1);
    wht_com1_send(WHT_COM1, tx_buffer, 5);

    while (1) {
        if (1 == receive_flag) {
            // 已经接受到数据，返回数据地址和数据长度
            for (tx_count = 0; tx_count < rx_count; tx_count++) {
                while (RESET == usart_flag_get(WHT_COM1, USART_FLAG_TBE));
                usart_data_transmit(WHT_COM1, com1_rxbuffer[tx_count]);
            }
            receive_flag = 0;
        }

        // feed watchdog

        // 1. Wireless rx check task
        // 1.1 Frame parse task
        // 1.2 Excute by frame type

        // 2. Data collect task
        // 2.1 Collect wire data

        // 3. Data pack task
        // 3.1 Pack 0x01: Join reply
        // 3.2 Pack 0x02: Wire test data

        // 4. Data tx task
        // 4.1 Send 0x01: Join reply
        // 4.2 Send 0x02: Wire test data

        // 5. Device status check task

        // 6. Pdt test task
    }
}

void timer_task(void) {
    heartbeat++;
    gpio_bit_toggle(GPIOC, GPIO_PIN_6);  // 切换LED状态
    wire_data = gpio_input_port_get(GPIOE);
}

void com_rx_idle_callback(void) {
    rx_count = 256 - (dma_transfer_number_get(DMA0, DMA_CH5));
    receive_flag = 1;
}

void running_led_init(void) {
    /* enable the LEDs GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOC);

    /* configure LED2 GPIO port */
    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_6);
    /* reset LED2 GPIO pin */
    gpio_bit_reset(GPIOC, GPIO_PIN_6);
}

// init the GPIOE for the wire data input
int wire_gpio_init(void) {
    rcu_periph_clock_enable(RCU_GPIOE);
    gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_ALL);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_ALL);
    return 0;
}
