#include "slave_mode.h"

#include "wht_timer.h"

void running_led_init(void);
void slave_timer_config(void);
void timer_task(void);
int wire_gpio_init(void);

uint16_t wire_data = 0;

int slave_mode_entry(void) {
    systick_config();
    running_led_init();
    timer1_init(timer_task);
    wire_gpio_init();

    while (1);
}

void timer_task(void) {
    gpio_bit_toggle(GPIOC, GPIO_PIN_6);  // 切换LED状态
    wire_data = gpio_input_port_get(GPIOE);
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
