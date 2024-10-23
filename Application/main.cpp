
#include <cstdint>
#ifdef __cplusplus
extern "C" {
#endif

#include "com.h"
#include "led.h"
#include "systick.h"

#ifdef __cplusplus
}
#endif

extern uint8_t usart1_rx_count;
extern uint8_t usart2_rx_count;

int main(void) {
    systick_config();

    LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);

    uint8_t data_to_send[] = {0x01, 0x02, 0x03};

    Serial1 serial1(usart1_rx_count);
    serial1.init(115200);
    serial1.data_send(data_to_send, 1);
    serial1.dma_tx(data_to_send, 2);

    Serial2 serial2(usart2_rx_count);
    serial2.init(115200);
    serial2.data_send(data_to_send, 1);
    serial2.dma_tx(data_to_send, 2);

    while (1) {
        // led0.toggle();
        if (serial1.rx_count > 0) {
            serial1.dma_tx(serial1.rxbuffer, serial1.rx_count);
            serial1.rx_count = 0;
        }
        
        if (serial2.rx_count > 0) {
            serial2.dma_tx(serial2.rxbuffer, serial2.rx_count);
            serial2.rx_count = 0;
        }
    }
}