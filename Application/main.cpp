
#include <cstdint>
#include <cstdlib>
#include <cstring>
#ifdef __cplusplus
extern "C" {
#endif

#include "systick.h"

#ifdef __cplusplus
}
#endif
#include "com.h"
#include "led.h"
#include "timer.h"
#include "json_interface.h"

LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);

extern SerialConfig usart1_config;
extern SerialConfig usart2_config;
Serial com1(usart1_config);

void ledTask() { led0.toggle(); }

int main(void) {
    systick_config();

    FrameParser parser;

    uint8_t data_to_send[] = {0x01, 0x02, 0x03};

    com1.data_send(data_to_send, 1);
    com1.dma_tx(data_to_send, 2);

    Serial com2(usart2_config);
    com2.data_send(data_to_send, 1);
    com2.dma_tx(data_to_send, 2);

    Timer timer;
    timer.init(50, ledTask);

    while (1) {
        if (usart1_config.rx_count > 0) {
            std::vector<uint8_t> vec(com1.rxbuffer, com1.rxbuffer + usart1_config.rx_count);
            parser.FrameParse(vec);
            usart1_config.rx_count = 0;
        }
    }
}