#include "slave_mode.h"

#include "wht_timer.h"

uint16_t wire_data = 0;
static int32_t heartbeat = -1;

void running_led_init(void);
void slave_timer_config(void);
void timer_task(void);
int wire_gpio_init(void);

int slave_mode_entry(void) {
    systick_config();
    running_led_init();
    timer1_init(timer_task);
    wire_gpio_init();

    while (1) {
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
