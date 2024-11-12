#include "main.h"

#include "inc/log.h"

#define WH_NODE
// LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);

extern SerialConfig usart1_config;
extern Serial com1;

FrameFragment assembled_frame;
FrameFragment frame_fragment;

int main(void) {
    systick_config();
    std::array<uint8_t, 4> device_uid;
    uid::get(device_uid);
#ifdef WH_NODE
    ChronoLink chronoLink;
    for (;;) {
        if (usart1_config.rx_count > 0) {
            DBGF("Recv data\n");
            // FIXME: data copy is wrong cause vector is not contiguous
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
