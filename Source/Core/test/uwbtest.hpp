#ifndef _TransparentTransmission_HPP
#define _TransparentTransmission_HPP
#include <cstdint>
#include <vector>

#include "Taskcpp.h"
#include "bsp_gpio.hpp"
#include "uwb.hpp"
#include "uwb_interface.hpp"
#include "bsp_log.hpp"
extern Logger Log;

#define TX_MODE   0
#define RX_MODE   1
#define TEST_MODE RX_MODE
class TransparentTransmissionTest : public TaskClassS<1024> {
   public:
    TransparentTransmissionTest() : TaskClassS("ttt", TaskPrio_High),
    pin(GPIO::Port::D, GPIO::Pin::PIN_1, GPIO::Mode::OUTPUT) {}
    ~TransparentTransmissionTest() {}

   private:
    struct header {
        uint16_t seq;
    };
    GPIO pin;
    void task() override {
        uint16_t recv_cnt = 0;
        header h;
        
        pin.bit_set();
        std::vector<uint8_t> data;

#if TEST_MODE == TX_MODE
        UWB<UwbUartInterface> uwb;
        for (uint8_t i = 0; i < 122; i++) {
            data.push_back(i);
        }
        for (;;) {
            for (uint16_t i = 0; i < 1000; i++) {
                // h.seq = i;
                // data[0] = h.seq >> 8;
                // data[1] = h.seq & 0xff;
                uwb.data_transmit(data);
                // pin.bit_set();
                // vTaskDelay(pdMS_TO_TICKS(2));
                // pin.bit_reset();
            }
            // pin.bit_set();
            // vTaskDelay(pdMS_TO_TICKS(200));
            // pin.bit_reset();
        }
#else
        UWB<DevBoardInterface> uwb;
        uwb.set_recv_mode();
        for (;;) {
            if (uwb.get_recv_data(data)) {
                recv_cnt++;
                // Log.r(data.data(), data.size());
                
            }
            if (recv_cnt == 100) {
                recv_cnt = 0;
                pin.bit_set();
                vTaskDelay(pdMS_TO_TICKS(1));
                pin.bit_reset();
            }
            // vTaskDelay(pdMS_TO_TICKS(1));
        }
#endif
    }
};

#endif