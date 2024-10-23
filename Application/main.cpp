
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
int main(void)
{
    systick_config();

    LED led0(RCU_GPIOC, GPIOC, GPIO_PIN_6);

    // 创建类的实例
    UsartCom com1(usart1_rx_count);

    // 初始化串口，设置波特率为115200
    com1.init(115200);

    // 模拟要发送的数据
    uint8_t data_to_send[] = {0x01, 0x02, 0x03};

    // 通过DMA发送数据
    com1.data_send(data_to_send, 1);
    com1.dma_tx(data_to_send, 2);

    while (1) {
        // led0.toggle();
        if (com1.rx_count > 0) {
            com1.dma_tx(com1.rxbuffer, com1.rx_count);
            com1.rx_count = 0;
        }
    }
}