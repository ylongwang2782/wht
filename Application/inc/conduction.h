#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#include "gd32f4xx.h"

#ifdef __cplusplus
}
#endif

class Conduction {
   public:
    struct GpioPin {
        uint32_t port;
        uint32_t pin;
    };

    static constexpr std::array<GpioPin, 10> pin_map = {{{GPIOE, GPIO_PIN_0},
                                                         {GPIOE, GPIO_PIN_1},
                                                         {GPIOE, GPIO_PIN_2},
                                                         {GPIOE, 1 << 3},
                                                         {GPIOE, 1 << 4},
                                                         {GPIOE, 1 << 5},
                                                         {GPIOE, 1 << 6},
                                                         {GPIOE, 1 << 7},
                                                         {GPIOE, 1 << 8},
                                                         {GPIOE, 1 << 9}}};
    std::vector<uint8_t> result;
    std::vector<uint8_t> collect_pin_states();
    void config(uint8_t count);
    void start();
    bool data_get(uint8_t *data);
    void master_pin_set(uint8_t pin_num);
    void master_pin_reset(uint8_t pin_num);

   private:
    uint8_t enabled_pin_num;
    uint8_t master_pin_index = 0;
    uint32_t packed_data = 0;
    int bit_position = 0;
};