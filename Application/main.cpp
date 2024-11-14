#include "main.h"

#include "conduction.h"

#define WH_NODE
extern SerialConfig usart1_config;
extern Serial com1;
extern Conduction conduction;

LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);
FrameFragment frame_fragment;
Timer timer(50);
ChronoLink chronoLink;

void onTimerInterrupt() {
    led0.toggle();
    conduction.collect_pin_states();
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
