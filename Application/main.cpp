
#include <cstdint>
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
LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);

void ledTask() { led0.toggle(); }

extern SerialConfig usart1_config;
extern SerialConfig usart2_config;
extern uint16_t usart1_rx_count;
extern uint16_t usart2_rx_count;

int main(void) {
    systick_config();

    uint8_t data_to_send[] = {0x01, 0x02, 0x03};

    Serial com1(usart1_config);
    com1.data_send(data_to_send, 1);
    com1.dma_tx(data_to_send, 2);

    Serial com2(usart2_config);
    com2.data_send(data_to_send, 1);
    com2.dma_tx(data_to_send, 2);

    Timer timer;
    timer.init(50, ledTask);

    while (1) {
        if (usart1_rx_count > 0) {
            com1.dma_tx(com1.rxbuffer, usart1_rx_count);
            usart1_rx_count = 0;
        }

        if (usart2_rx_count > 0) {
            com2.dma_tx(com2.rxbuffer, usart2_rx_count);
            usart2_rx_count = 0;
        }
    }
}