#include "conduction.h"

#include <cstdint>
#include <cstdio>

#include "led.h"
#include "log.h"
#include "timer.h"

extern "C" {
#include "systick.h"
}

extern Timer timer;

Conduction conduction;

void Conduction::config(DeviceConfigInfo devConf) {
    // enable clock for corresponding GPIO port
    rcu_periph_clock_enable(RCU_GPIOE);
    // reset all pins
    for (size_t i = 0; i < devConf.devConductionPinNum; i++) {
        master_pin_reset(i);
    }
    devConductionPinNum = devConf.devConductionPinNum;
    DBGF("dev cond pin num: %d\n", devConductionPinNum);
    sysConductionPinNum = devConf.sysConductionPinNum;
    DBGF("sys cond pin num: %d\n", sysConductionPinNum);
    deviceCount = devConf.deviceCount;
    DBGF("sys device count: %d\n", devConf.deviceCount);
}

void Conduction::start() { timer.start(); }

// TODO: optimize store format u32 to u8
uint8_t Conduction::collect_pin_states() {
    // row index
    if (matrix.row_index < sysConductionPinNum) {
        // host pin: high ouput and no store
        // if (matrix.startCol < matrix.row_index < matrix.col +
        // matrix.startCol) {
        //     master_pin_set(matrix.row_index);
        // }

        for (matrix.col_index = 0; matrix.col_index < matrix.col;
             matrix.col_index++) {
            // col index
            const auto& gpio_pin = pin_map[matrix.col_index];

            if (bit_position > 0 && bit_position % 8 == 0) {
                result.push_back(packed_data);  // store per 8 bits
                packed_data = 0;
                bit_position = 0;
            }

            if (matrix.col_index + matrix.startCol != matrix.row_index) {
                packed_data <<= 1;
                if (gpio_input_bit_get(gpio_pin.port, gpio_pin.pin) == SET) {
                    packed_data |= 0x01;  // set highest bit
                }
                ++bit_position;
            }   
        }

        matrix.row_index++;
    } else {
        // Store rest of data
        if (bit_position > 0) {
            packed_data <<= (8 - bit_position);
            result.push_back(packed_data);
        }
        matrix.row_index = 0;
        packed_data = 0;
        bit_position = 0;
        timer.stop();
        return 1;  // 1 means collect data done, then go to upload stage
    }
    return 0;
}

bool Conduction::data_get(uint8_t* data) { return true; }

// PE0 - PE16 are the used pins. when pin_num = 0, set PE0 as master which is
// output high, pin_num = 1, enable PE1, and so on.
// Master Pin: ouput high
void Conduction::master_pin_set(uint8_t pin_num) {
    const GpioPin& selected_pin = pin_map[pin_num];
    gpio_mode_set(selected_pin.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  selected_pin.pin);
    gpio_output_options_set(selected_pin.port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            selected_pin.pin);
}

// Master Pin Reset: set pin as input and reset it
void Conduction::master_pin_reset(uint8_t pin_num) {
    const GpioPin& selected_pin = pin_map[pin_num];
    gpio_mode_set(selected_pin.port, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
                  selected_pin.pin);
}
