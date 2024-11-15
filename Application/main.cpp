#include "main.h"

#include <cstdint>

#include "conduction.h"

#define WH_NODE
extern SerialConfig usart1_config;
extern Serial com1;
extern Conduction conduction;

LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);
FrameFragment frame_fragment;
Timer timer(50);
ChronoLink chronoLink;

uint8_t is_data_upload = 0;

void onTimerInterrupt() {
    led0.toggle();
    if (conduction.collect_pin_states()) {
        // Data Collect done, ready to upload
        chronoLink.is_data_upload = true;
    }
}

int main(void) {
    systick_config();
    std::array<uint8_t, 4> device_uid;
    uid::get(device_uid);
#ifdef WH_NODE
    Timer::registerInstance(&timer);
    timer.setCallback(onTimerInterrupt);
    for (;;) {
        if (usart1_config.rx_count > 0) {
            DBGF("Recv data\n");
            chronoLink.receiveData(com1.rxbuffer, usart1_config.rx_count);
            while (chronoLink.parseFrameFragment(frame_fragment)) {
                DBGF("Recv frame fragment\n");
                chronoLink.receiveAndAssembleFrame(frame_fragment);
            };
            usart1_config.rx_count = 0;
        }

        if (chronoLink.is_data_upload) {
            chronoLink.is_data_upload = false;
            // If conduction result is not empty, pack and upload
            if (!conduction.result.empty()) {
                std::vector<std::vector<uint8_t>> upload_buf;
                ChronoLink::pack(1, ChronoLink::CONDUCTION_DATA,
                                 conduction.result.data(),
                                 conduction.result.size(), upload_buf);
                conduction.result.clear();
                // Print upload buf
                for (auto& buf : upload_buf) {
                    for (auto& b : buf) {
                        printf("%02X ", b);
                    }
                };
            }
        }
    }
#endif

#ifdef WH_AP
    FrameParser parser;
    for (;;) {
        if (usart1_config.rx_count > 0) {
            parser.JsonParse((char*)com1.rxbuffer);
            usart1_config.rx_count = 0;
        }
    }
#endif
}
