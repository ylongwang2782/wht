#include "harness.h"

#include <cstdint>
#include <cstdio>

#include "bsp_led.h"
#include "bsp_log.h"

extern Logger Log;

Harness harness;

void Harness::init() {
    // enable clock for corresponding GPIO port
    rcu_periph_clock_enable(RCU_GPIOE);
    // reset all pins
    for (size_t i = 0; i < pin_map.size(); i++) {
        slave_pin_set(i);
    }
}

uint8_t Harness::collect_pin_states() {
    for (matrix.col_index = 0; matrix.col_index < matrix.col;
         matrix.col_index++) {
        const auto& gpio_pin = pin_map[matrix.col_index];
        result.push_back(gpio_input_bit_get(gpio_pin.port, gpio_pin.pin));
    }
    return 0;
}

bool Harness::data_get(uint8_t* data) { return true; }

// PE0 - PE16 are the used pins. when pin_num = 0, set PE0 as master which is
// output high, pin_num = 1, enable PE1, and so on.
// Master Pin: ouput high
void Harness::master_pin_set(uint8_t pin_num) {
    const GpioPin& selected_pin = pin_map[pin_num];
    gpio_mode_set(selected_pin.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  selected_pin.pin);
    gpio_output_options_set(selected_pin.port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            selected_pin.pin);
}

// Master Pin Reset: set pin as input and reset it
void Harness::slave_pin_set(uint8_t pin_num) {
    const GpioPin& selected_pin = pin_map[pin_num];
    gpio_mode_set(selected_pin.port, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
                  selected_pin.pin);
}
