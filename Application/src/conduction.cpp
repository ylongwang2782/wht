#include "conduction.h"
std::array<uint16_t, 16> conduction_data;

void Conduction::run(uint8_t pin_num) {
    rcu_periph_clock_enable(RCU_GPIOE);
    for (size_t i = 0; i < Conduction::pin_map.size(); i++) {
        master_pin_reset(i);
    }
    auto results = collect_pin_states(pin_num);
    // Print conduction data to check
    for (auto hex_value : results) {
        printf("%08X\n", hex_value);
    }
}

std::vector<uint32_t> Conduction::collect_pin_states(uint8_t count) {
    std::vector<uint32_t> result;
    uint32_t packed_data = 0;
    int bit_position = 0;

    for (int i = 0; i < count; i++) {
        const auto& gpio_pin = pin_map[i];
        if (bit_position > 0 && bit_position % 32 == 0) {
            result.push_back(packed_data);  // 每32位保存一次
            packed_data = 0;
            bit_position = 0;
        }

        // fill highest bit first, move then fill
        packed_data >>= 1;
        if (gpio_input_bit_get(gpio_pin.port, gpio_pin.pin) == SET) {
            packed_data |= 0x80000000;  // 将最高位设置为1
        }

        ++bit_position;
    }

    // Store rest of data
    if (bit_position > 0) {
        packed_data >>= (32 - bit_position);  // 右移补齐高位填充
        result.push_back(packed_data);
    }

    return result;
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
